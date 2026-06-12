Args = Struct(
    args: List( String, Type )
);

Type = Struct(
        type_id: TypeId
        size: USz
        align: USz
);

Fn = Struct(
        ast: SynNode
        free_vars: List( String, Type )
        ret: Option(Type)
);

Struct = Fn( args: ArgsType, ret: Type, body: {
        type_id = GLOBAL_TYPE_INFO.reserve();
        type = Type(
                type_id,
                size: calculated(),
                align: calculated(),
        );
        GLOBAL_TYPE_INFO.append(TypeInfo(
                fields: List( arg: String, offset: USize, trait impls or smthn, bound methods maybe ),
                default_constructor: Fn( args: CalculatedArgs(), ret: type, body: { 
                        mem = allocate( calculated() ),
                } )
        ));
        type
} );

foo = Fn( args: Args(count: USize), ret: USize, body: { count } );
