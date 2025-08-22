// My intent is to write a cp -> c compiler. just syntax sugar really, but a language server and type checker would also be nice and that's the spicy part.
// Let the syntax speak for itself.

struct Foo {
        bar: i32
}
// oh yeah. this is just syntax sugar but is 70% the reason for the project. the other 30% is modules.
impl Foo {
        // desguars foo.new() to something like Foo::new() to just ::proj::Foo::new(), which in turn becomes __proj__Foo__new in the global namespace.
        fn new() -> Foo {
                Foo { bar: 0 }
        }
        // desugars foo1.add(foo2) to something like Foo::inc(foo1, foo2). 
        fn add(self, foo: Foo) {
                self.bar += foo.bar
        }

}


fn main(void) -> i32 {
        // screw auto
        let x = 1;
        let y = 2;
        let z = malloc(y * 8);
        // defer keyword. maybe we could do a drop impl as well but.
        defer free(z);


}

mod baz {
        // default is pub(crate). put it in a header for pub - headers and modules? probably fine. prefix with a __ for private?
        fn inner()

}

/* Type builtins:
 * 
 * _i8, _i16, _i32, _i64, _i128, _isize
 * _u8, _u16, _u32, _u64, _u128, _usize, _uptr // usize is the diff of two uptrs. its the size of the address space, not the size of a pointer.
 * _f32, _f64, 
 * str, [T], [T;n], ()
 * struct, union, enum
 * fn(T1, T2, .. ) -> U
 * *T
 * std library ifdefs these to drop the underscore, but its a macro library on purpose, so users can easily re-def them.
 */
