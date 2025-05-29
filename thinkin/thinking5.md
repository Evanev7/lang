woah woah
let's talk about rust tasks

I want something nicer than rust, but i think i should start with rust.

```rust
task! PhysicsUpdate {
    struct Position {
        x: f32,
        y: f32,
    }
    struct Velocity {
        x: f32,
        y: f32,
    }
    struct Extra {
        custom: Option<fn(delta f32) -> ()>
    }
    uniform {
        delta: f32
    }

    # Inherits uniforms (statics etc.)
    fn update(pos Position, vel Velocity) -> ()
        pos += vel * delta
    fn process(ex Extra) -> ()
        match ex:custom {
            Some(fn) -> fn(delta),
            None -> {},
        }
}
```

compiling to

```rust
struct PhysicsUpdate {
    position: Vec<Position>,
    velocity: Vec<Velocity>,
    extra: Vec<Extra>
    uniform: {
        delta: f32
    }
}
impl PhysicsUpdate {
    fn _update(&self, pos: &mut Position, vel: &mut Velocity) {
        pos += vel * self.uniform.delta
    }
    fn update(&mut self) {
        # cool question about mutable borrows here
        # also about multithreading
        position.par_iter().zip(velocity.par_iter()).map(PhysicsUpdate::_update)
    }
}
```