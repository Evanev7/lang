module arena;
uses std;

struct Arena {
        USize       capacity;
        USize       used;
        Option<U1*> buf;
}

fn Arena Arena_new(USize bytes);
fn void  Arena_free(Arena* arena);

fn void* Arena_alloc(Arena arena, USize size, USize align);
