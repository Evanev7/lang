use std::marker::PhantomData;

use flume::{Receiver, Sender};

use crate::plan::Plan;

pub trait Strategy: Sized {
    type Error: From<crate::Error>;
    type Task: Task<Strategy = Self>;
    type TaskBuilder;
    //type AsyncTask: Task<Strategy = Self>;
    type BuildContext: BuildContext;
    type ExecutionContext: ExecutionContext;
    fn prepare(plan: Plan<Self>) -> Self;
    fn execute() -> Result<(), Self::Error>;

    fn add_stage<F>(&mut self, f: F);
    fn add_async_stage<F>(&mut self, f: F);
}

pub struct FlumeExecutor {
    
}

pub trait Task {
    type Strategy: Strategy<Task = Self>;
    fn execute(
        &self,
        ctx: <Self::Strategy as Strategy>::ExecutionContext,
    ) -> Result<(), <Self::Strategy as Strategy>::Error>;
}
pub trait TaskBuilder {
    type Strategy: Strategy<TaskBuilder = Self>;
    fn build(
        self: Box<Self>,
        ctx: &mut <Self::Strategy as Strategy>::BuildContext,
    ) -> <Self::Strategy as Strategy>::Task;
}

pub struct TaskBuilderStruct<F, I, O>
where
    F: Fn(I) -> O,
{
    f: F,
    _input_marker: PhantomData<I>,
    _output_marker: PhantomData<O>,
}
impl TaskBuilder for TaskBuilderStruct<>

pub struct TaskStruct<F, I, O>
where
    F: Fn(I::Types) -> O::Types,
    I: Connection,
    O: Connection,
{
    f: F,
    inbound_data: I::Receivers,
    _output_marker: O::Senders,
}

pub trait Connection {
    type Receivers;
    type Senders;
    type Types;
}

pub trait BuildContext {}

pub trait ExecutionContext {}
