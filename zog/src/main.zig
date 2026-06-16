pub fn main() !void {
    std.debug.print("All your {s} are belong to us.\n", .{"codebase"});
    std.debug.print("{any}", .{lib.tokenize("yoo")});
}

const std = @import("std");
const lib = @import("zog_lib");
