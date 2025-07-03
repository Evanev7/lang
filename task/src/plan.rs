use std::{any::Any, future::Future, marker::PhantomData};

use petgraph::graph::DiGraph;

use crate::strategy::Strategy;

/// The Plan is the "compilers" IR - a graph of nodes between stages or other plans, or resources.
pub struct Plan<S: Strategy> {
    graph: DiGraph<PlanNode<S>, PlanConnection>,
}

pub(crate) enum PlanNode<S: Strategy> {
    Task(S::Task),
    //AsyncTask(S::AsyncTask),
    //Service(Box<dyn Service>)
    Resource(Resource),
}
pub(crate) enum PlanConnection {
    ResourceAccess { kind: ResourceAccessKind },
    // This should not exist at runtime, and be replaced by a Channel<()> if no channel exists otherwise.
    Ordering {},
    Channel {},
}

pub(crate) enum ResourceAccessKind {
    Shared,
    Unique,
    //Semaphore?
}

pub(crate) enum Resource {
    Data(Box<dyn Any + Send>),
    //Semaphore(std::sync::atomic::AtomicUsize, AnyData),
}

#[cfg(test)]
mod test {}
