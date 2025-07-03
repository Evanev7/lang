# Styx

Styx is a programming model that aims to extend flowgraph and actor systems to create a highly performant, simple, concurrent API. Here's the rundown:

A graph is built at compile time.
 - Tasks - synchronous, cpu bound work
 - AsyncTasks - Async, io bound work
 - Resources - Shared, potentially mutable state

Additionally, it's edges are either
 - Resource-to-Task: Scheduler acquired leases - either & or &mut access to the resource for the duration of the task, undirected.
 - Task-to-Task: Channels. Data is transferred between tasks, and a scheduler message is sent - the scheduler checks if all dependencies are met before queuing the task (a data push model.)