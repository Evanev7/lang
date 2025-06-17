use std::{any::Any, collections::HashMap, marker::PhantomData};

use petgraph::graph::DiGraph;

pub struct ExecutionGraph<'a> {
    graph: DiGraph<inner::Node, inner::Access>,
    _marker: PhantomData<fn() -> &'a ()>,
}

impl<'a> ExecutionGraph<'a> {
    pub fn new() -> Self {
        Self {
            graph: DiGraph::new(),
            _marker: PhantomData,
        }
    }
    pub fn add_input<T: Clone + 'static>(&mut self, name: &'static str, data: T) -> Handle<'a, T> {
        let node = self
            .graph
            .add_node(inner::Node::InputData(inner::InputData {
                name,
                data: Box::new(data),
            }));
        Handle::new(node)
    }
    pub fn add_task<F, I, O>(&mut self, name: &'static str, inputs: I, logic: F) -> Handle<'a, O>
    where
        F: for<'b> Fn(I::Data<'b>) -> O + 'static,
        I: inner::TaskData,
        O: 'static,
    {
        let node = self.graph.add_node(inner::Node::Task(inner::Task {
            name,
            op: Box::new(move |erased_inputs: inner::ErasedInputs| {
                let data = I::ret(erased_inputs).unwrap();
                let out = logic(data);
                Box::new(out)
            }),
            deps: inputs.dependents(),
        }));
        for dep in inputs.dependents() {
            self.graph.add_edge(dep.idx, node, dep.access);
        }
        Handle::new(node)
    }
}

pub trait GraphExector {
    fn execute<'a, T: 'static>(
        &mut self,
        graph: &ExecutionGraph<'a>,
        handle: Handle<'a, T>,
    ) -> Result<T, &'static str>;
}

pub struct LinearExecutor;
impl GraphExector for LinearExecutor {
    fn execute<'a, T: 'static>(
        &mut self,
        graph: &ExecutionGraph<'a>,
        handle: Handle<'a, T>,
    ) -> Result<T, &'static str> {
        let nodes = petgraph::algo::toposort(&graph.graph, None)
            .map_err(|_| "Cycle detected in execution graph")?;
        let mut result_store = HashMap::<petgraph::graph::NodeIndex, Box<dyn Any>>::new();
        for node_index in nodes {
            match &graph.graph[node_index.clone()] {
                inner::Node::InputData(data) => {
                    let foo = result_store.insert(node_index, data.data.clone_box());
                    assert!(foo.is_none());
                }
                inner::Node::Task(task) => {
                    let mut read_backing_store: Vec<Box<dyn Any>> = vec![];
                    let mut read_write_backing_store: Vec<Box<dyn Any>> = vec![];
                    task.deps.iter().for_each(|dep| {
                        let item = result_store.remove(&dep.idx).expect("Where'd ma data go?");
                        match dep.access {
                            inner::Access::Read => read_backing_store.push(item),
                            inner::Access::ReadWrite => read_write_backing_store.push(item),
                        }
                    });
                    let read_data: Vec<&dyn Any> =
                        read_backing_store.iter().map(Box::as_ref).collect();
                    let mut read_write_data: Vec<&mut dyn Any> = read_write_backing_store
                        .iter_mut()
                        .map(Box::as_mut)
                        .collect();
                    let erased_inputs = inner::ErasedInputs {
                        read: &read_data,
                        write: &mut read_write_data,
                    };
                    let task_output = (task.op)(erased_inputs);
                    result_store.insert(node_index, task_output);
                    let mut read_iter = read_backing_store.into_iter();
                    let mut write_iter = read_write_backing_store.into_iter();
                    task.deps.iter().for_each(|dep| {
                        let item: Box<dyn Any>;
                        match dep.access {
                            inner::Access::Read => item = read_iter.next().expect("huhhh???"),
                            inner::Access::ReadWrite => item = write_iter.next().expect("huhh??!"),
                        }
                        result_store.insert(dep.idx, item);
                    });
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
    node: petgraph::graph::NodeIndex,
    _marker: PhantomData<T>,
    _graph_id: PhantomData<&'a ()>,
}
impl<'a, T> Handle<'a, T> {
    fn new(node: petgraph::graph::NodeIndex) -> Self {
        Handle {
            node,
            _marker: PhantomData,
            _graph_id: PhantomData,
        }
    }
}

mod inner {
    use petgraph::graph::NodeIndex;

    use super::*;

    pub(crate) enum Access {
        Read,
        ReadWrite,
    }
    pub struct Dependency {
        pub(crate) idx: NodeIndex,
        pub(crate) access: Access,
    }

    pub struct ErasedInputs<'b> {
        pub(crate) read: &'b [&'b dyn Any],
        pub(crate) write: &'b mut [&'b mut dyn Any],
    }

    pub trait TaskData {
        type Data<'b>;
        fn dependents(&self) -> Vec<Dependency>;
        fn ret<'b>(item: ErasedInputs<'b>) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized;
    }
    impl TaskData for () {
        type Data<'b> = ();
        fn dependents(&self) -> Vec<Dependency> {
            vec![]
        }
        fn ret<'b>(item: ErasedInputs<'b>) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized,
        {
            if !(item.read.len() == 0 && item.write.len() == 0) {
                return Err("Task which takes no inputs called with data");
            }
            Ok(())
        }
    }
    impl<'a, T: 'static> TaskData for Read<Handle<'a, T>> {
        type Data<'b> = &'b T;
        fn dependents(&self) -> Vec<Dependency> {
            vec![Dependency {
                idx: self.0.node,
                access: Access::Read,
            }]
        }
        fn ret<'b>(item: ErasedInputs<'b>) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized,
        {
            if !(item.read.len() == 1 && item.write.len() == 0) {
                return Err("Task which takes one input called with many");
            }
            // Length checked - index is safe.
            item.read[0]
                .downcast_ref()
                .ok_or_else(|| "Task produced output of incorrect type")
        }
    }
    impl<'a, T: 'static> TaskData for ReadWrite<Handle<'a, T>> {
        type Data<'b> = &'b mut T;
        fn dependents(&self) -> Vec<Dependency> {
            vec![Dependency {
                idx: self.0.node,
                access: Access::ReadWrite,
            }]
        }
        fn ret<'b>(item: ErasedInputs<'b>) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized,
        {
            if !(item.read.len() == 0 && item.write.len() == 1) {
                return Err("Task which takes one input called with many");
            }
            // Length checked - index is safe.
            item.write[0]
                .downcast_mut()
                .ok_or_else(move || "Task produced output of incorrect type")
        }
    }
    impl<'a, A, B> TaskData for (Read<Handle<'a, A>>, Read<Handle<'a, B>>)
    where
        A: 'static,
        B: 'static,
    {
        type Data<'b> = (&'b A, &'b B);
        fn dependents(&self) -> Vec<Dependency> {
            vec![
                Dependency {
                    idx: self.0 .0.node,
                    access: Access::Read,
                },
                Dependency {
                    idx: self.1 .0.node,
                    access: Access::Read,
                },
            ]
        }
        fn ret<'b>(item: ErasedInputs<'b>) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized,
        {
            if !(item.read.len() == 2 && item.write.len() == 0) {
                return Err("Task called with incorrect number of inputs");
            }
            let i1 = item.read[0]
                .downcast_ref::<A>()
                .ok_or_else(|| "Task produced output of incorrect type")?;
            let i2 = item.read[1]
                .downcast_ref::<B>()
                .ok_or_else(|| "Task produced output of incorrect type")?;
            Ok((i1, i2))
        }
    }
    impl<'a, A, B> TaskData for (Read<Handle<'a, A>>, ReadWrite<Handle<'a, B>>)
    where
        A: 'static,
        B: 'static,
    {
        type Data<'b> = (&'b A, &'b B);
        fn dependents(&self) -> Vec<Dependency> {
            vec![
                Dependency {
                    idx: self.0 .0.node,
                    access: Access::Read,
                },
                Dependency {
                    idx: self.1 .0.node,
                    access: Access::ReadWrite,
                },
            ]
        }
        fn ret<'b>(item: ErasedInputs<'b>) -> Result<Self::Data<'b>, &'static str>
        where
            Self: Sized,
        {
            if !(item.read.len() == 1 && item.write.len() == 1) {
                return Err("Task called with incorrect number of inputs");
            }
            let i1 = item.read[0]
                .downcast_ref::<A>()
                .ok_or_else(|| "Task produced output of incorrect type")?;
            let i2 = item.write[0]
                .downcast_mut::<B>()
                .ok_or_else(|| "Task produced output of incorrect type")?;
            Ok((i1, i2))
        }
    }

    pub(crate) trait AnyClone: Any {
        fn clone_box(&self) -> Box<dyn Any>;
    }
    impl<T: 'static + Any + Clone> AnyClone for T {
        fn clone_box(&self) -> Box<dyn Any> {
            Box::new(self.clone())
        }
    }

    pub(crate) enum Node {
        Task(Task),
        InputData(InputData),
    }

    pub(crate) struct InputData {
        pub(crate) name: &'static str,
        pub(crate) data: Box<dyn AnyClone>,
    }
    pub(crate) struct Task {
        pub(crate) name: &'static str,
        pub(crate) op: Box<dyn for<'b> Fn(ErasedInputs<'b>) -> Box<dyn Any>>,
        pub(crate) deps: Vec<Dependency>,
    }
}

#[cfg(test)]
mod test {
    use super::*;
    #[test]
    fn test_simple_linear_chain() {
        let mut graph = ExecutionGraph::new();

        let initial_value = graph.add_input("an int", 10i32);
        let plus_five = graph.add_task("plus_five", Read(initial_value), |x| x + 5);
        let times_two = graph.add_task("times_two", Read(plus_five), |x| x * 2);

        let result = LinearExecutor.execute(&graph, times_two).unwrap();
        // (10 + 5) * 2 == 30
        assert_eq!(result, 30);
    }
    #[test]
    fn test_diamond_execution() {
        let mut graph = ExecutionGraph::new();

        let initial_value = graph.add_input("an int", 10i32);
        let plus_five = graph.add_task("plus_five", Read(initial_value), |x| x + 5);
        let times_two = graph.add_task("times_two", Read(initial_value), |x| x * 2);
        let to_string =
            graph.add_task("to_string", (Read(plus_five), Read(times_two)), |(x, y)| {
                format!("{} + {} = {}", x, y, x + y)
            });

        let result = LinearExecutor.execute(&graph, to_string).unwrap();

        assert_eq!(result, "15 + 20 = 35");
    }
    #[test]
    fn test_read_write_dependency() {
        let mut graph = ExecutionGraph::new();

        let val_handle = graph.add_input("an int", 100i32);

        // The first task mutates the input. The task itself produces no meaningful
        // output (it returns ()), but its side-effect on `val_handle` is what we test.
        let increment_task = graph.add_task("increment", ReadWrite(val_handle), |x: &mut i32| {
            *x += 10;
        });

        // The second task reads from the *original* data handle (`val_handle`),
        // but it has a dependency on `increment_task`. The scheduler must run
        // `increment_task` first, so this read will see the mutated value.
        // To enforce this, we also pass the handle from the increment task. Since it produces
        // `()`, we just use a `Read` on it to establish the dependency edge.
        let read_after_write = graph.add_task(
            "read_after_write",
            (Read(val_handle), Read(increment_task)), // depends on both
            |data: (&i32, &())| *data.0,              // Read the integer, ignore the ()
        );

        let mut executor = LinearExecutor;
        let result = executor.execute(&graph, read_after_write).unwrap();

        // The value should be 100 + 10 = 110
        assert_eq!(result, 110);
    }
    #[test]
    fn test_no_input_task() {
        let mut graph = ExecutionGraph::new();

        let generate_forty_two = graph.add_task("generate", (), |_| 42);

        let mut executor = LinearExecutor;
        let result = executor.execute(&graph, generate_forty_two).unwrap();

        assert_eq!(result, 42);
    }
}
