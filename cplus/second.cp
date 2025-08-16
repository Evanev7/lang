// The simplest/most important parts:

module system:
        I think I actually want C style header files for my modules, I just think they're neat. I can unity build anyway.
        typedef struct Foo {} Foo; is fixable, pretty easily actually.
        I don't want to write mod_submod_list_push(List, thing), I want to write List.push(thing) or list::push(List, thing)

defer:
        this is just syntactic sugar. should be easy.

struct T:
        should be possible in vanilla C

header files:
        I think this is better an alternative to pub/priv, but should still be automated.


impl blocks:
        This is essentially just creating a module for every type. the main question is supporing foo.bar, which requires type checking to parse.
        perhaps we use ~ syntax?
        foo.bar() <- associated function
        foo~bar   <- offset
