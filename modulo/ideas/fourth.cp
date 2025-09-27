Ok, let's crystalise this

We have the usual datatypes, but I'm feeling freaky so I'm calling them:
U1 U2 U4 U8 U16 USize UPtr
I1 I2 I4 I8 I16 ISize

Args is a special datatype. It's a built in List + Map pair, so we don't need a distinct List / Map at comptime.
Conceptually,
Args<Type> = struct<
        pos_args = List<Type>
        kw_args = Map<Str, Type>
>

Fn = struct<
        const_args = Args
        runtime_args = Args
        ret = Type
        body = str // or an AST
>
We also have a runtime Function, which is opaque.

Final builtin is
Type = struct<
        size = USize
        align = U1 // Measured in powers of two? so 1 = 1 bit, 4 = 1 byte, 7 = 8 byte, 10 = 64 bytes. Cannot see us ever needing much more than 4 bits here tbh.
        accessors? = Map<Str, (int, Type)> // Unordered - ordering is after compilation
        default = Option<Box<Self>> // Uniform representation of default value, defaults to None. None default types MUST be provided values (no zero-init)
>

We have a nice fn sugar:

foo = fn< ..const_args >( ..args ) -> ..ret {
        body
}

conceptually speaking, all functions are either const_args -> value or const_args -> runtime args -> value. 
struct = fn< **kwargs = Map<Str, Type> > -> Type

foo< -- const function call, returns a compile time value
foo( -- runtime function call, returns a runtime value
foo[ -- indexing
foo{ -- block, perhaps kotlin style closure passing.
foo < -- comparison - whitespace sensitive - true of all operators, they require whitespace, so
foo+ -- illegal
foo + -- addition

the eventual goal is proof engines as a language construct.
the type system will be one such comptime proof engine, with libraries capable of providing their own
