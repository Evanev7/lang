"Write slow code, and let CPUs speed up"
came from a presentation in 2012. I don't think CPUs have sped up since then, but they have gotten wider, less sequential and more predictive.

I think they main point, to me, is that modern compilers rely very heavily on link-time optimization (C++) and single translation units (Rust) to aggressively inline.
This feels, to me, like a "final" optimization. We essentially evaluate the program at compile time as much as humanly possible - you can't go up from here.

I want a new foundation that unlocks new optimizations - mainly in data oriented design. I propose that memory layout should not be a strict guarantee of the compiler,
and that LTO should be almost completely non existent. Libraries should be able to be compiled and linked against, rather than recompiled.

Want to optimize the physics engine? modify and recompile it, and you're done (etc. etc.)

What does this mean:
Monomorphization and other compile time goodies
 - I would like to address this kind of thing with a livepatching system, to add new monomorphizations (only if necessary), potentially providing runtime polymorphism
 - I would also like to minimize the amount of function calling across library boundaries. Perhaps an actual distinction between "function" and "task" libraries.
  - For this discussion I will call function libraries 'crates', as they align with Rust crates.
  - I would prefer that libraries interoperate through 'tasks', and aim to allow all that tasty inlining to occur at compile time of the (task) library
   - This means we need to write syntax to encourage patterns such as loop unswitching
Memory model and C
 - We're explicitly against a global _synchronous_ state.
 - The CPU is an asynchronous processor, and data races are.. not the right problem to be worrying about?