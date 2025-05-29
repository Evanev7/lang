set:
    x: int
    y: int



distinguishing between:
Data Type
Actual Data

Actual Data is constructed and tangible, a Data Type is a set of Actual Datas.

We want labelled, rather than ordered ADTS:
struct Foo {
    foo: Int
    fie: Int
    bar: struct {
        baz: String
        bap: Song
    }
}
could be
Foo = Int * Int * (String * Song) (foo, fie, bar: (baz, bap))
But then it makes much more sense to
Foo = (foo: Int, fie: Int, bar: (baz: String, bap: Song))
and then some syntaxing sorts the things


Ugly syntax:

enum Foo {
    foo: Int
    fie: String
    Bar1 {
        feg: Int
    }
    Bar2 {
        feg: Yunt
        fog: String
    }
}

so we can have a Foo::Bar1 or a Foo::Bar2. a Foo (type) can always access Foo.foo or Foo.fie, but only a Foo::Bar1 can access feg etc.
As a set, this is a
Foo = Int * String * (Int |_| (Yunt * String))
