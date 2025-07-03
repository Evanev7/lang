//! A stage is the compile time equivalent of a task, and gets converted into a task at build time.

use std::{future::Future, marker::PhantomData, pin::Pin};

use crate::{
    strategy::{BuildContext, Strategy},
    task::Task,
};

pub trait Stage {
    type Strategy: Strategy;
    fn build(self, ctx: &mut <Self::Strategy as Strategy>::BuildContext);
}
pub trait AsyncStage {
    type Strategy: Strategy;
    fn build(self, ctx: &mut <Self::Strategy as Strategy>::BuildContext);
}

pub struct StageStruct<F, I, O> {
    f: F,
    i: PhantomData<I>,
    o: PhantomData<O>,
}
impl<F, I, O> StageStruct<F, I, O> {
    fn new(f: F) -> Self {
        Self {
            f,
            i: PhantomData,
            o: PhantomData,
        }
    }
}

impl<F, I, O> Stage for StageStruct<F, I, O>
where
    F: Fn(I) -> O,
{
    fn build(self, ctx: &mut <Self::Strategy as Strategy>::BuildContext) {
        ctx.build(self)
    }
}

// Services later...
