need to narrow the focus; how are we going to _restrict_ programmers in order to _give them what they want_?
we're targeting *C#* with *Rust* semantics and hopefully one way interop.

- Error handling:
> Result<T>
> Error type is a [Dynamic Union](dynamic_unions)?

- Traits or similar:
> Trait for behaviour
> Components for data


## Dynamic Unions
Essentially, gathers at compile time the tagged error types into a union. Consider:
```Rust
fn foo(): Result[String] > -- dw im not sold on this syntax. 
    if .rand > -- this either, mostly wrt properties, obj.thing is _so_ well known. good for getters and setters though.
        IoError::FileNotFound
    else >
        "Okay!"
fn propaate(): Result[Int] >
    .foo.map{
        if .len > 10 >
            Size::TooLarge
        else >
            .len
    }
```
We read `foo()` and determine that it's a `foo(): () -> Result[String, IoError::FileNotFound | Size::TooLarge]` and let you match against those.