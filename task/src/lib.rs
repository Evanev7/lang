#![allow(unused)]
use std::{any::Any, collections::HashMap, marker::PhantomData};

use petgraph::{
    graph::{DiGraph, NodeIndex},
    visit::EdgeRef,
};

pub struct ExecutionGraph<'a> {
    graph: DiGraph<inner::Node, inner::Connection>,
    _marker: PhantomData<fn() -> &'a ()>,
}

impl<'a> ExecutionGraph<'a> {
    pub fn new() -> Self {
        Self {
            graph: DiGraph::new(),
            _marker: PhantomData,
        }
    }
    pub fn add_input<T: Clone + Send + Sync + 'static>(&mut self, data: T) -> Handle<'a, T> {
        let node = self
            .graph
            .add_node(inner::Node::InputData(inner::InputData {
                data: Box::new(data),
            }));
        Handle::new(node)
    }
    pub fn add_stage<F, I, O>(&mut self, inputs: I, logic: F) -> Handle<'a, O>
    where
        F: for<'b> Fn(I::Data<'b>) -> O + 'static,
        I: inner::IntoDependencySet,
        O: 'static + Send + Sync,
    {
        let node = self.graph.add_node(inner::Node::Stage(inner::Stage {
            op: Box::new(move |erased_inputs| {
                let data = I::to_data(erased_inputs).unwrap();
                let out = logic(data);
                Box::new(out)
            }),
        }));
        for dep in inputs.dependencies() {
            self.graph.add_edge(dep.node_idx, node, dep.connection);
        }
        Handle::new(node)
    }
}

pub trait GraphExecutor {
    fn execute<T: 'static>(
        &mut self,
        graph: &ExecutionGraph,
        handle: Handle<T>,
    ) -> Result<T, &'static str>;
}

pub type ErasedData = Box<dyn Any + Send + Sync>;

pub struct LinearExecutor;
impl GraphExecutor for LinearExecutor {
    fn execute<'a, T: 'static>(
        &mut self,
        graph: &ExecutionGraph,
        handle: Handle<T>,
    ) -> Result<T, &'static str> {
        let nodes = petgraph::algo::toposort(&graph.graph, None)
            .map_err(|_| "Cycle detected in execution graph")?;
        let mut result_store = HashMap::<NodeIndex, ErasedData>::new();
        for node_index in nodes {
            match &graph.graph[node_index] {
                inner::Node::InputData(data) => {
                    let foo = result_store.insert(node_index, data.data.clone_box());
                    assert!(foo.is_none());
                }
                inner::Node::Stage(stage) => {
                    let mut inbound_edges = graph
                        .graph
                        .edges_directed(node_index, petgraph::Direction::Incoming)
                        .into_iter()
                        .collect::<Vec<_>>();
                    inbound_edges.sort_unstable_by_key(|edge| edge.weight().arg_idx);
                    let mut rw_backing_store: Vec<(NodeIndex, ErasedData)> = vec![];
                    let mut data_refs: Vec<inner::AnyRef> = vec![];
                    for edge in &inbound_edges {
                        if let inner::ConnectionKind::ReadWrite = edge.weight().kind {
                            rw_backing_store.push((
                                edge.source(),
                                result_store
                                    .remove(&edge.source())
                                    .expect("oh no the data wasn't real"),
                            ));
                        }
                    }
                    let mut rw_iter = rw_backing_store.iter_mut();
                    for edge in inbound_edges {
                        match edge.weight().kind {
                            inner::ConnectionKind::Read => data_refs.push(inner::AnyRef::ReadRef(
                                result_store
                                    .get(&edge.source())
                                    .expect("oh no the data wasn't real")
                                    .as_ref(),
                            )),
                            inner::ConnectionKind::ReadWrite => {
                                data_refs.push(inner::AnyRef::ReadWriteRef({
                                    rw_iter
                                        .next()
                                        .expect("oh no the data wasn't real")
                                        .1
                                        .as_mut()
                                }))
                            }
                        }
                    }
                    let out = (stage.op)(data_refs.as_mut_slice());
                    for (idx, box_) in rw_backing_store {
                        assert!(
                            result_store.insert(idx, box_).is_none(),
                            "oh no the data WAS real"
                        )
                    }
                    result_store.insert(node_index, out);
                }
            }
            if node_index == handle.node {
                return Ok(*result_store
                    .remove(&handle.node)
                    .unwrap()
                    .downcast()
                    .unwrap());
            }
        }
        Err("huh??")
    }
}

pub struct Read<T>(pub T);
pub struct ReadWrite<T>(pub T);
#[derive(Clone, Copy)]
pub struct Handle<'a, T> {
    node: NodeIndex,
    _type_marker: PhantomData<T>,
    _graph_marker: PhantomData<&'a ()>,
}
impl<'a, T> Handle<'a, T> {
    fn new(node: NodeIndex) -> Self {
        Handle {
            node,
            _type_marker: PhantomData,
            _graph_marker: PhantomData,
        }
    }
}

mod inner {
    use petgraph::graph::NodeIndex;

    use super::*;

    pub enum AnyRef<'b> {
        ReadRef(&'b dyn Any),
        ReadWriteRef(&'b mut dyn Any),
    }

    // The information required to wire a node to this node.
    pub struct Dependency {
        pub(crate) node_idx: NodeIndex,
        pub(crate) connection: Connection,
    }

    /// The edge of a graph, connecting two nodes.
    pub(crate) struct Connection {
        pub(crate) kind: ConnectionKind,
        pub(crate) arg_idx: usize,
    }

    /// The kind of connection being instantiated - buffer access, or channel.
    pub(crate) enum ConnectionKind {
        Read,
        ReadWrite,
        //Channel((SenderWrapper, ReceiverWrapper)),
    }
    pub trait IntoDependencyInfo {
        type Data<'b>;
        fn dependency(&self) -> Dependency;
        fn to_data<'b>(item: &'b mut AnyRef<'b>) -> Result<Self::Data<'b>, &'static str>;
    }
    impl<'a, T: 'static> IntoDependencyInfo for Read<Handle<'a, T>> {
        type Data<'b> = &'b T;
        fn dependency(&self) -> Dependency {
            Dependency {
                node_idx: self.0.node,
                connection: Connection {
                    kind: ConnectionKind::Read,
                    arg_idx: 0,
                },
            }
        }
        fn to_data<'b>(item: &'b mut AnyRef<'b>) -> Result<Self::Data<'b>, &'static str> {
            if let &mut AnyRef::ReadRef(r) = item {
                r.downcast_ref().ok_or("uh oh")
            } else {
                Err("uh oh")
            }
        }
    }
    impl<'a, T: 'static> IntoDependencyInfo for ReadWrite<Handle<'a, T>> {
        type Data<'b> = &'b mut T;
        fn dependency(&self) -> Dependency {
            Dependency {
                node_idx: self.0.node,
                connection: Connection {
                    kind: ConnectionKind::ReadWrite,
                    arg_idx: 0,
                },
            }
        }
        fn to_data<'b>(item: &'b mut AnyRef<'b>) -> Result<Self::Data<'b>, &'static str> {
            if let AnyRef::ReadWriteRef(ref mut r) = item {
                r.downcast_mut().ok_or("uh oh")
            } else {
                Err("uh oh")
            }
        }
    }
    pub trait IntoDependencySet {
        type Data<'b>;
        fn dependencies(&self) -> Vec<Dependency>;
        fn to_data<'b>(item: &'b mut [AnyRef<'b>]) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized;
    }
    impl IntoDependencySet for () {
        type Data<'b> = ();
        fn dependencies(&self) -> Vec<Dependency> {
            vec![]
        }
        fn to_data<'b>(item: &'b mut [AnyRef<'b>]) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized,
        {
            if item.len() > 0 {
                return Err(":(");
            }
            Ok(())
        }
    }
    impl<T: IntoDependencyInfo> IntoDependencySet for T {
        type Data<'b> = T::Data<'b>;
        fn dependencies(&self) -> Vec<Dependency> {
            // unpack self into a tuple "T1, T2" etc.
            #[allow(non_snake_case)]
            let t = self;
            let mut deps = Vec::new();
            let mut arg_idx = 0;
            deps.push(t.dependency());
            arg_idx += 1;

            deps
        }
        fn to_data<'b>(item: &'b mut [AnyRef<'b>]) -> Result<Self::Data<'b>, &'static str> {
            let mut i = 0;
            Ok({
                let data = T::to_data(&mut item[i])?;
                i += 1;
                data
            })
        }
    }

    macro_rules! into_dependency_set_impl {
        ( $($T:ident),+) => {
            impl<$($T: IntoDependencyInfo),+> IntoDependencySet for ($($T,)+) {
                type Data<'b> = ($($T::Data<'b>,)+);
                fn dependencies(&self) -> Vec<Dependency> {
                    // unpack self into a tuple "T1, T2" etc.
                    #[allow(non_snake_case)]
                    let ($($T,)+) = self;
                    let mut deps = Vec::new();
                    let mut arg_idx = 0;
                    // compile time for loop.
                    $(
                        let mut dep = $T.dependency();
                        dep.connection.arg_idx = arg_idx;
                        deps.push(dep);
                        arg_idx += 1;
                    )+
                    deps
                }
                fn to_data<'b>(item: &'b mut [AnyRef<'b>]) -> Result<Self::Data<'b>, &'static str> {
                    let mut iter = item.iter_mut();
                    Ok(($({
                        let elem = iter.next().ok_or("uh oh")?;
                        let data = $T::to_data(elem)?;
                        data },)+))
                }

            }
        };
    }
    into_dependency_set_impl!(T1);
    into_dependency_set_impl!(T1, T2);
    into_dependency_set_impl!(T1, T2, T3);
    into_dependency_set_impl!(T1, T2, T3, T4);

    pub(crate) trait AnyClone: Any + Send + Sync {
        fn clone_box(&self) -> ErasedData;
    }
    impl<T: 'static + Any + Clone + Send + Sync> AnyClone for T {
        fn clone_box(&self) -> ErasedData {
            Box::new(self.clone())
        }
    }

    pub(crate) enum Node {
        Stage(Stage),
        InputData(InputData),
    }

    pub(crate) struct InputData {
        pub(crate) data: Box<dyn AnyClone>,
    }
    pub(crate) struct Stage {
        pub(crate) op: Box<dyn for<'b> Fn(&'b mut [AnyRef<'b>]) -> ErasedData>,
    }
}

#[cfg(test)]
mod test {
    use super::*;
    #[test]
    fn test_simple_linear_chain() {
        let mut graph = ExecutionGraph::new();

        let initial_value = graph.add_input(10i32);
        let plus_five = graph.add_stage(Read(initial_value), |x| x + 5);
        let times_two = graph.add_stage(Read(plus_five), |x| x * 2);

        let result = LinearExecutor.execute(&graph, times_two).unwrap();
        // (10 + 5) * 2 == 30
        assert_eq!(result, 30);
    }
    #[test]
    fn test_diamond_execution() {
        let mut graph = ExecutionGraph::new();

        let initial_value = graph.add_input(10i32);
        let plus_five = graph.add_stage(Read(initial_value), |x| x + 5);
        let times_two = graph.add_stage(Read(initial_value), |x| x * 2);
        let to_string = graph.add_stage((Read(plus_five), Read(times_two)), |(x, y)| {
            format!("{} + {} = {}", x, y, x + y)
        });

        let result = LinearExecutor.execute(&graph, to_string).unwrap();

        assert_eq!(result, "15 + 20 = 35");
    }
    #[test]
    fn test_read_write_dependency() {
        let mut graph = ExecutionGraph::new();

        let val_handle = graph.add_input(100i32);

        let increment_stage = graph.add_stage(ReadWrite(val_handle), |x: &mut i32| {
            *x += 10;
        });

        // We add a dependency on the increment_stage to ensure this runs after the write happens.
        let read_after_write = graph.add_stage(
            (Read(val_handle), Read(increment_stage)), // depends on both
            |data: (&i32, &())| *data.0,               // Read the integer, ignore the ()
        );

        let mut executor = LinearExecutor;
        let result = executor.execute(&graph, read_after_write).unwrap();

        // 100 + 10 = 110
        assert_eq!(result, 110);
    }
    #[test]
    fn test_no_input_stage() {
        let mut graph = ExecutionGraph::new();

        let generate_forty_two = graph.add_stage((), |_| 42);

        let mut executor = LinearExecutor;
        let result = executor.execute(&graph, generate_forty_two).unwrap();

        assert_eq!(result, 42);
    }
}
