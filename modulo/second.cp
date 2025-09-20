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

do we need pointers?
        We need memory locations and the ability to define them
        But once we have a memory location, we can index into it instead of pointing
        What is the difference?
        - indices are safe (contiguous, length checked)
        - indices are fast (much easier to pick U1-U8)
        - pointers get functions, those can't go in a list
                - but most function pointers are kind of lies?
                - perhaps. polymorphism question
        - polymorphism is cool. would we want a bits type for doing bithacking?
        - pointer access is self contained - this is good and bad
        - handles can still drift out of sync just like pointers can, it's just less bad
        

