So, we want comptime stuff.
Types only available at comptime
Function pointers and lambdas are interesting.
Do we want to approach the "inline assembly in python" problem?
Maybe a special "Function Store"? Or just Function stores as a concept?
- this doesn't really solve the memory issues. what data _is_ a function?
- it is known at comptime (mem regions) but optimization changes sizes wildly
- alternatively, statically linked with arbitrary data - this is pure syntax
  sugar and should be buildable on whatever system i decide on

I want easy parsing? I don't know why I care about this so much but I really do
<> for comptime
() for runtime
[] for indexing?
{} for scopes & blocks
; vs \n vs ',' vs,,,, something else

exposed comptime functions are all available at runtime, obvs.
some types (like Type) are explicitly not

rust does do variadic generics through tuples.
i do like python *args **kwargs but some parts are crungo
maybe a typed args / typed kwargs situation

foo.bar for member access
prefix all functions? :print(), mod:print()?
its kinda noisy - and what about closures and fn pointers - I want both.
need a good middle ground where parsing can tell the difference between
function calls and data access, but


I want strong mut knowledge - data races still possible in this lang
But we are building the blocks of Resources - not Tasks - from Styx
What _is_ "using" data like?

So, let's define this mem function that instantiates a Resource

fn Type<str> -> Type?
fn Arena<T> -> Type?

fn mem<T: Type, cfg>(cfg) -> Arena<T>

storig = mem<T>()
handle = storig.insert(Point<x: 1, y: 2>)


// These are types
Foo = struct<name = Type, name = Type>

Point = struct<
        x = F4
        y = F4
>

p = Point(x=1,y=2)

fn struct<kws: Map<str, Type>> -> Type

Union = union<
        foo = Foo
        pt = Point
>
FooEnum = enum<
        FOO = 0
        BAR
>
FooUnion = struct<tag = FooEnum, data = union<foo = Foo>>

All functions have signature that is one of
f :: const_stuff -> (runtime_stuff -> runtime_value)
h :: const_stuff -> const_value
Type and fn are always
Only runtime stuff and source code can cross the sig boundary
All runtime_stuff can be used as const_stuff, args can be permuted - "const overloaded"
so f(x) = x+2 has f<2> = 4

f(x) is shorthand for f<>(x)

potato = Fn<|args = Type| {
        1 + 1
        2 + 2
        3
}>

We're currently looking very functional: everything is a value, just some types are const. I like the simplicity
As for parsing complexity, determining if something is a function call or not is easy?? perhaps
NAME< -- indicates a static function call 
NAME < -- indicates comparison
NAME( -- indicates a function call
NAME[ -- indicates an indexable value
NAME. -- indicates a subscriptable value - a subfield
NAME~ -- something else
NAME+
NAME/
NAME-
NAME*.-- deref
NAME* -- multiplication
NAME! -- macro? maybe something else
NAME? -- try or null safety (kotlin, rust)

add = Fn<
        args = FnArgs<
                static = [U8<x>, U8<y>]
                runtime = []
                return = U8
        >
        body = Ast<{
                x + y
        }>
>
// We can assign comptime values freely since the binding is just namespacing?
Bar~val=Fn<args = FnArgs<static = [], runtime = [self], return = U8>
some_u8:U8=Bar.val()

// Parameterized types
Foo<T> = struct<
        t = T
>

// Define a struct type at const time
Point = struct< x = F64, y = F64 >

// Define an enum type
Color = enum< Red = 0, Green = 1, Blue = 2 >

// Struct with nested fields
Pixel = struct< p = Point, c = Color >

len_limit = Fn<
  params = [ N = USize, s = Str ],
  return = Bool,
  body   = {
    s.len <= N
  }
>
fn<N: USize>(s: Str) -> Bool {
  s.len <= N
}
// Specialize with const param N=64
check64 = len_limit<64>

// Call at runtime
ok = check64("hello")

