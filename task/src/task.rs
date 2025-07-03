use std::{future::Future, marker::PhantomData, pin::Pin};

pub trait Task {
    type Err: From<crate::Error>;
    fn execute(&self) -> Result<(), Self::Err>;
}
pub trait AsyncTask {
    type Err: From<crate::Error>;
    fn execute(&self) -> Pin<Box<dyn Future<Output = Result<(), Self::Err>>>>;
}

pub struct TaskStruct<F, I, O> {
    f: F,
    i: PhantomData<I>,
    o: PhantomData<O>,
}
impl<F, I, O> TaskStruct<F, I, O> {
    fn new(f: F) -> Self {
        Self {
            f,
            i: PhantomData,
            o: PhantomData,
        }
    }
}

// Services later...
