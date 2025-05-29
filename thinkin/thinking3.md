Lambdas and Async

I really like kotlin `it` for maps, I think

[1, 2, 3]
    .map { it + 1 }

is very elegant. 

The rust equivalent is

[1, 2, 3]
    .iter()
    .map(|x| {x+1})

specifying args is nice once we grow beyond one arg.

The syntaxless monad allows us to simply apply |x| {x+1} to [1,2,3]:

[1, 2, 3]
    .|x| {x+1}

Or, supposing

fn plus_one(x: int): int {
    x+1
}

[1, 2, 3]
    .plus_one

I think I'm saying `.` is an invocation and `::` is access, so 2.f = 3, 2::f = Fn(Int) -> Int

[1, 2, 3]
    + 1 // [2, 3, 4]
    * 4 // [8, 12, 16]
    .append 4 // [8, 12, 16, 4]
    .rotate // [4, 8, 12, 16]
