#![allow(unused)]
pub mod legacy;

use kanal::{Receiver, Sender};
use petgraph::{
    graph::{DiGraph, NodeIndex},
    visit::{EdgeRef, IntoNeighborsDirected},
};
use std::{
    any::{Any, TypeId},
    marker::PhantomData,
    ops::{Deref, DerefMut, Index},
    sync::{RwLock, RwLockReadGuard, RwLockWriteGuard},
};

pub struct Executor {
    graph: DiGraph<Node, Edge>,
}
impl Executor {
    pub fn new() -> Self {
        Self {
            graph: DiGraph::new(),
        }
    }

    pub fn add_task<F, I, O, D>(&mut self, handles: I, f: F) -> TaskHandle<O>
    where
        F: for<'a> Fn(I::Data<'a>) -> O + 'static,
        O: Clone + 'static,
        I: Args<Receivers = D> + 'static,
        D: ArgsState,
    {
        let mut receivers = vec![];
        for (handle, _) in handles.get_edge_info() {
            if let Node::Task(ref mut t) = self.graph[handle] {
                receivers.push(t.receiver())
            }
        }
        let node_index = self
            .graph
            .add_node(Node::task::<F, I, O>(f, D::downcast(receivers)));
        for (handle, connection) in handles.get_edge_info() {
            self.graph.add_edge(handle, node_index, connection);
        }
        TaskHandle {
            idx: node_index,
            _marker: PhantomData,
        }
    }

    pub fn add_resource<T>(&mut self, data: T) -> ResourceHandle<T>
    where
        T: Any,
    {
        let node_index = self.graph.add_node(Node::resource(data));
        ResourceHandle {
            idx: node_index,
            _marker: PhantomData,
        }
    }
    #[must_use]
    pub fn execute(&mut self) -> Result<(), ExecutionError> {
        let Ok(nodes) = petgraph::algo::toposort(&self.graph, None) else {
            return Err(ExecutionError::CyclicGraph);
        };
        for node in nodes {
            let Node::Task(task) = &self.graph[node] else {
                continue;
            };
            let rw_guards = self.acquire_guards(node);
            match task.poll(rw_guards) {
                Ok(()) => Ok(()),
                Err(e) => unreachable!("Scheduled incorrectly! {:?}", e),
            }?
        }
        Ok(())
    }
    // I un-elided the lifetimes here for my own understanding.
    fn acquire_guards<'a>(&'a self, node: NodeIndex) -> RwGuards<'a> {
        let mut refs = vec![];
        for edge in self
            .graph
            .edges_directed(node, petgraph::Direction::Incoming)
        {
            if let Node::Resource(rw_lock) = &self.graph[edge.source()] {
                refs.push((rw_lock, edge.weight()));
            }
        }
        refs.sort_unstable_by_key(|(_, k)| k.arg_idx);
        let refs = refs.into_iter().map(|(k, _)| k).collect();
        RwGuards { refs }
    }
    pub fn get<'a, T>(&'a self, resource_handle: ResourceHandle<T>) -> Option<ReadGuard<'a, T>> {
        let Node::Resource(ref resource) = &self.graph[resource_handle.idx] else {
            return None;
        };
        Some(ReadGuard {
            guard: resource.read().ok()?,
            _marker: PhantomData,
        })
    }
    pub fn get_mut<'a, T>(
        &'a mut self,
        resource_handle: ResourceHandle<T>,
    ) -> Option<WriteGuard<'a, T>> {
        let Node::Resource(ref mut resource) = &mut self.graph[resource_handle.idx] else {
            return None;
        };
        Some(WriteGuard {
            guard: resource.write().ok()?,
            _marker: PhantomData,
        })
    }
}
use inner::*;
mod inner {
    use std::sync::RwLockReadGuard;

    use super::*;

    /// arguments to a task. Data is the actual type of the arguments,
    /// state is the objects needed to reconstruct the arguments - channel receivers.
    /// dependencies tell use where to source the arguments and
    pub trait Args {
        type Data<'a>;
        type Receivers;
        fn get_edge_info(&self) -> Vec<(NodeIndex, Edge)>;
        fn prepare_inputs<'a>(
            receivers: &Self::Receivers,
            rw_guards: &'a mut RwGuards,
        ) -> Result<Self::Data<'a>, ReceiveError>;
    }

    impl Args for () {
        type Data<'a> = ();
        type Receivers = ();
        fn get_edge_info(&self) -> Vec<(NodeIndex, Edge)> {
            vec![]
        }
        fn prepare_inputs<'a>(
            receivers: &Self::Receivers,
            rw_guards: &'a mut RwGuards,
        ) -> Result<Self::Data<'a>, ReceiveError> {
            Ok(())
        }
    }

    impl<T> Args for TaskHandle<T> {
        type Data<'a> = T;
        type Receivers = Receiver<T>;
        fn get_edge_info(&self) -> Vec<(NodeIndex, Edge)> {
            vec![(
                self.idx,
                Edge {
                    arg_idx: 0,
                    meta: Access::Consume,
                },
            )]
        }
        fn prepare_inputs<'a>(
            state: &Self::Receivers,
            ctx: &'a mut RwGuards,
        ) -> Result<Self::Data<'a>, ReceiveError> {
            match state.try_recv() {
                Ok(Some(v)) => Ok(v),
                Ok(None) => Err(ReceiveError::Empty),
                Err(kanal::ReceiveError::Closed) => Err(ReceiveError::Closed),
                Err(kanal::ReceiveError::SendClosed) => Err(ReceiveError::Closed),
            }
        }
    }
    impl<T: 'static> Args for Read<ResourceHandle<T>> {
        type Data<'a> = ReadGuard<'a, T>;
        type Receivers = ();
        fn get_edge_info(&self) -> Vec<(NodeIndex, Edge)> {
            vec![(
                self.0.idx,
                Edge {
                    arg_idx: 0,
                    meta: Access::Read,
                },
            )]
        }
        fn prepare_inputs<'a>(
            state: &Self::Receivers,
            ctx: &'a mut RwGuards,
        ) -> Result<Self::Data<'a>, ReceiveError> {
            match ctx.refs[0].try_read() {
                Ok(guard) => Ok(ReadGuard {
                    guard,
                    _marker: PhantomData,
                }),
                Err(std::sync::TryLockError::WouldBlock) => Err(ReceiveError::WouldBlock),
                Err(_) => panic!("Resource poisoned"),
            }
        }
    }
    impl<T: 'static> Args for Write<ResourceHandle<T>> {
        type Data<'a> = WriteGuard<'a, T>;
        type Receivers = ();
        fn get_edge_info(&self) -> Vec<(NodeIndex, Edge)> {
            vec![(
                self.0.idx,
                Edge {
                    arg_idx: 0,
                    meta: Access::Write,
                },
            )]
        }
        fn prepare_inputs<'a>(
            state: &Self::Receivers,
            ctx: &'a mut RwGuards,
        ) -> Result<Self::Data<'a>, ReceiveError> {
            match ctx.refs[0].try_write() {
                Ok(guard) => Ok(WriteGuard {
                    guard,
                    _marker: PhantomData,
                }),
                Err(std::sync::TryLockError::WouldBlock) => Err(ReceiveError::WouldBlock),
                Err(_) => panic!("Resource poisoned"),
            }
        }
    }

    // maybe should just be From<>, idk.
    pub trait ArgsState {
        fn downcast(receivers: Vec<Box<dyn Any>>) -> Self;
    }
    impl ArgsState for () {
        fn downcast(receivers: Vec<Box<dyn Any>>) -> Self {
            assert!(receivers.len() == 0);
            ()
        }
    }
    impl<T: 'static> ArgsState for (Receiver<T>) {
        fn downcast(mut receivers: Vec<Box<dyn Any>>) -> Self {
            assert!(receivers.len() == 1);
            *receivers.pop().unwrap().downcast().expect("cringe")
        }
    }

    pub struct RwGuards<'a> {
        pub(crate) refs: Vec<&'a RwLock<Box<dyn Any>>>,
    }
    pub struct Edge {
        pub(crate) arg_idx: usize,
        pub(crate) meta: Access,
    }
    pub enum Access {
        Consume,
        Read,
        Write,
    }
}
#[derive(Debug)]
pub enum ReceiveError {
    Closed,
    Empty,
    WouldBlock,
}
#[derive(Debug, Clone, Copy)]
pub struct TaskHandle<T> {
    pub(crate) idx: NodeIndex,
    pub(crate) _marker: PhantomData<T>,
}
impl<T> TaskHandle<T> {
    fn new(idx: NodeIndex) -> Self {
        Self {
            idx,
            _marker: PhantomData,
        }
    }
}
#[derive(Debug, Clone, Copy)]
pub struct ResourceHandle<T> {
    pub(crate) idx: NodeIndex,
    pub(crate) _marker: PhantomData<T>,
}
impl<T> ResourceHandle<T> {
    fn new(idx: NodeIndex) -> Self {
        Self {
            idx,
            _marker: PhantomData,
        }
    }
}
#[derive(Debug)]
pub struct Read<T>(T);
#[derive(Debug)]
pub struct Write<T>(T);
#[derive(Debug)]
pub struct ReadGuard<'a, T> {
    guard: RwLockReadGuard<'a, Box<dyn Any>>,
    _marker: PhantomData<&'a T>,
}
impl<'a, T: 'static> Deref for ReadGuard<'a, T> {
    type Target = T;
    fn deref(&self) -> &Self::Target {
        self.guard
            .downcast_ref()
            .expect("Task scheduled with incorrect arguments. CRITICAL LIBRARY BUG")
    }
}
#[derive(Debug)]
pub struct WriteGuard<'a, T> {
    guard: RwLockWriteGuard<'a, Box<dyn Any>>,
    _marker: PhantomData<&'a T>,
}
impl<'a, T: 'static> Deref for WriteGuard<'a, T> {
    type Target = T;
    fn deref(&self) -> &Self::Target {
        self.guard
            .downcast_ref()
            .expect("Task scheduled with incorrect arguments. CRITICAL LIBRARY BUG")
    }
}
impl<'a, T: 'static> DerefMut for WriteGuard<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.guard
            .downcast_mut()
            .expect("Task scheduled with incorrect arguments. CRITICAL LIBRARY BUG")
    }
}

pub(crate) enum NodeType {
    Task,
    Resource,
}
pub(crate) enum Node {
    Task(Box<dyn TaskNode>),
    Resource(RwLock<Box<dyn Any>>),
}
impl Node {
    pub(crate) fn task<F, I, O>(f: F, receivers: I::Receivers) -> Self
    where
        F: for<'a> Fn(I::Data<'a>) -> O + 'static,
        I: Args + 'static,
        O: Clone + 'static,
    {
        let td = TaskData::<F, I, O>::new(f, receivers);
        Node::Task(Box::new(td))
    }

    pub(crate) fn resource<T: Any>(t: T) -> Self {
        Node::Resource(RwLock::new(Box::new(t)))
    }
}
pub(crate) trait TaskNode {
    fn poll(&self, ctx: RwGuards) -> Result<(), ReceiveError>;
    // ehh. TODO.
    fn receiver(&mut self) -> Box<dyn Any>;
}

pub(crate) struct TaskData<F, I: Args, O> {
    pub(crate) f: F,
    pub(crate) receivers: I::Receivers,
    pub(crate) senders: Vec<kanal::Sender<O>>,
}
impl<F, I, O> TaskNode for TaskData<F, I, O>
where
    F: for<'a> Fn(I::Data<'a>) -> O,
    I: Args,
    O: Clone + 'static,
{
    fn poll(&self, mut ctx: RwGuards) -> Result<(), ReceiveError> {
        let args = I::prepare_inputs(&self.receivers, &mut ctx)?;
        let ret = (self.f)(args);
        for sender in &self.senders {
            sender.send(ret.clone()).ok();
        }
        Ok(())
    }
    fn receiver(&mut self) -> Box<dyn Any> {
        let (sender, receiver) = kanal::bounded::<O>(10);
        self.senders.push(sender);
        Box::new(receiver)
    }
}
impl<F, I, O> TaskData<F, I, O>
where
    F: for<'a> Fn(I::Data<'a>) -> O + 'static,
    I: Args,
    O: Clone,
{
    pub(crate) fn new(f: F, receivers: I::Receivers) -> Self {
        Self {
            f,
            receivers,
            senders: vec![],
        }
    }
}
#[derive(Debug)]
pub enum ExecutionError {
    CyclicGraph,
}

#[cfg(test)]
mod test {
    use super::*;
    #[test]
    fn test_simple_store() {
        let mut graph = Executor::new();
        let buf = graph.add_resource(2);
        let plus_five = graph.add_task(Write(buf), |mut buf| *buf = 4);
        graph.execute().unwrap();
        assert_eq!(*graph.get(buf).unwrap(), 4);
    }
    #[test]
    fn test_linear_side_effects() {
        let mut graph = Executor::new();

        let initial_value = graph.add_resource(10i32);
        let plus_five = graph.add_task(Read(initial_value), |x| dbg!(*x + 5));
        let times_two = graph.add_task(plus_five, move |x| dbg!(x * 2));
        graph.execute().unwrap();
    }

    // Failing - need to figure out how I want to get graph outputs.
    #[test]
    fn test_simple_linear_chain() {
        let mut graph = Executor::new();

        let initial_value = graph.add_resource(10i32);
        let plus_five = graph.add_task(Read(initial_value), |x| *x + 5);
        let times_two = graph.add_task(plus_five, |x| x * 2);
        //let writer = graph.add_task((times_two, Write(initial_value)), |(x, r)| *r = x);
        graph.execute().unwrap();
        let result = *graph.get(initial_value).unwrap();
        // (10 + 5) * 2 == 30
        assert_eq!(result, 30);
    }

    /* Requires Args impl
    #[test]
    fn test_diamond_execution() {
        let mut graph = Executor::new();

        let initial_value = graph.add_resource(10i32);
        let plus_five = graph.add_task(Read(initial_value), |x| *x + 5);
        let times_two = graph.add_task(Read(initial_value), |x| *x * 2);
        let to_string = graph.add_task((Read(plus_five), Read(times_two)), |(x, y)| {
            format!("{} + {} = {}", x, y, x + y)
        });

        let result = LinearExecutor.execute(&graph, to_string).unwrap();

        assert_eq!(result, "15 + 20 = 35");
    }
    */

    /* Requires Args impl
    #[test]
    fn test_read_write_dependency() {
        let mut graph = Executor::new();

        let val_handle = graph.add_resource(100i32);

        let increment_stage = graph.add_task(Write(val_handle), |x| {
            *x += 10;
        });

        // We add a dependency on the increment_stage to ensure this runs after the write happens.
        let read_after_write = graph.add_task(
            (Read(val_handle), increment_stage), // depends on both
            |data| *data.0,                      // Read the integer, ignore the ()
        );

        graph.execute().unwrap();
        let result = *graph.get(val_handle).unwrap();

        // 100 + 10 = 110
        assert_eq!(result, 110);
    }
    */

    /* Requires Args impl
    #[test]
    fn test_no_input_stage() {
        let mut graph = Executor::new();

        let generate_forty_two = graph.add_task((), |_| 42);
        graph.execute().unwrap();
        let result = *graph.get(generate_forty_two).unwrap();

        assert_eq!(result, 42);
    }
    */
}
