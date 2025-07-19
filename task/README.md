# Styx

Styx is a programming model that aims to extend flowgraph and actor systems to create a highly performant, simple, concurrent API. Here's the rundown:

A graph is built at compile time, of the graph objects:
 - Tasks - synchronous, cpu bound work
 - AsyncTasks - Async, io bound work
 - Resources - Shared, potentially mutable state

Additionally, it's edges are either
 - Resource-to-Task: Scheduler acquired leases - either & or &mut access to the resource for the duration of the task, undirected.
 - Task-to-Task: Channels. Data is transferred between tasks, and a scheduler message is sent - the scheduler checks if all dependencies are met before queuing the task (a data push model.)


Ideal API:

```rust
// Somewhere in main.rs...
let plan = Plan::new();
let particles = plan.add_resource(initial_positions);
const GRAVITY = 9.81;
const DT = 0.16;
let update_gravity = plan.add_task(&mut particles, |pxs| pxs.vel.y -= GRAVITY * DT);
let update_velocity = plan.add_task(&mut particles, |pxs| pxs.pos += pxs.vel.y * DT)
                        .after(update_gravty);
let colliders = plan.add_task(&particles, collision_detection_fn).after(update_velocity);
let bounce = plan.add_task((&mut particles, colliders), |pxs, col_pairs| for pair in col_pairs {
    pxs.update_pair(do_some_algebra(pair));
})
plan.add_finite_feedback_edge(bounce, update_gravity, 100);
plan.build().execute();
```