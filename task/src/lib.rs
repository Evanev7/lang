#![allow(unused)]
pub mod legacy;

use kanal::{Receiver, Sender};
use petgraph::graph::{DiGraph, NodeIndex};
use std::{
    any::{Any, TypeId},
    marker::PhantomData,
    ops::{Deref, DerefMut},
    sync::{RwLock, RwLockReadGuard, RwLockWriteGuard},
};

pub struct Executor {
    graph: DiGraph<Node, Connection>,
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
        I: Args<State = D> + 'static,
        D: ArgsState,
    {
        let mut receivers = vec![];
        for (handle, _) in handles.dependencies() {
            if let Node::Task(ref mut t) = self.graph[handle] {
                receivers.push(t.receiver())
            }
        }
        let node_index = self
            .graph
            .add_node(Node::task::<F, I, O>(f, D::downcast(receivers)));
        for (handle, connection) in handles.dependencies() {
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

    pub fn execute(&mut self) -> Result<(), ExecutionError> {
        todo!();
        Ok(())
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
        type State;
        fn dependencies(&self) -> Vec<(NodeIndex, Connection)>;
        fn obtain<'a>(
            state: &Self::State,
            ctx: &'a mut Context,
        ) -> Result<Self::Data<'a>, ReceiveError>;
    }

    impl<T> Args for TaskHandle<T> {
        type Data<'a> = T;
        type State = Receiver<T>;
        fn dependencies(&self) -> Vec<(NodeIndex, Connection)> {
            vec![(self.idx, Connection::Task { arg_idx: 0 })]
        }
        fn obtain<'a>(
            state: &Self::State,
            ctx: &'a mut Context,
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
        type State = ();
        fn dependencies(&self) -> Vec<(NodeIndex, Connection)> {
            vec![(
                self.0.idx,
                Connection::Resource {
                    access: Access::Read,
                },
            )]
        }
        fn obtain<'a>(
            state: &Self::State,
            ctx: &'a mut Context,
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
        type State = ();
        fn dependencies(&self) -> Vec<(NodeIndex, Connection)> {
            vec![(
                self.0.idx,
                Connection::Resource {
                    access: Access::Write,
                },
            )]
        }
        fn obtain<'a>(
            state: &Self::State,
            ctx: &'a mut Context,
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

    pub struct Context<'a> {
        pub(crate) refs: Vec<&'a RwLock<Box<dyn Any>>>,
    }
    pub enum Connection {
        Task { arg_idx: usize },
        Resource { access: Access },
    }
    pub enum Access {
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
pub struct Read<T>(T);
pub struct Write<T>(T);
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
    pub(crate) fn task<F, I, O>(f: F, receivers: I::State) -> Self
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
    fn poll(&self, ctx: Context) -> Result<(), ExecutionError>;
    // ehh. TODO.
    fn receiver(&mut self) -> Box<dyn Any>;
}

pub(crate) struct TaskData<F, I: Args, O> {
    pub(crate) f: F,
    pub(crate) receivers: I::State,
    pub(crate) senders: Vec<kanal::Sender<O>>,
}
impl<F, I, O> TaskNode for TaskData<F, I, O>
where
    F: for<'a> Fn(I::Data<'a>) -> O,
    I: Args,
    O: Clone + 'static,
{
    fn poll(&self, mut ctx: Context) -> Result<(), ExecutionError> {
        let args = I::obtain(&self.receivers, &mut ctx).unwrap();
        let ret = (self.f)(args);
        for sender in &self.senders {
            sender.send(ret.clone());
        }
        Ok(())
    }
    fn receiver(&mut self) -> Box<dyn Any> {
        let (sender, receiver) = kanal::bounded::<O>(10);
        self.senders.push(sender);
        Box::new(receiver)
    }
}
impl<'a, F, I, O> TaskData<F, I, O>
where
    F: Fn(I::Data<'a>) -> O + 'static,
    I: Args,
    O: Clone,
{
    pub(crate) fn new(f: F, receivers: I::State) -> Self {
        Self {
            f,
            receivers,
            senders: vec![],
        }
    }
}

pub enum ExecutionError {}

#[cfg(test)]
mod test {
    use super::*;
    #[test]
    fn constructors() {
        let mut graph = Executor::new();
        let in_data = graph.add_resource([1, 2, 3]);
        graph.add_task(Write(in_data), |mut x| x[0] += 1);
        graph.execute();
        let node = &graph.graph[in_data.idx];
        let Node::Resource(r) = node else {
            panic!();
        };
        let b = r.read().unwrap();
        let b2 = b.downcast_ref::<[i32; 3]>().unwrap();
        assert_eq!(b2, &[2, 2, 3])
    }
}
