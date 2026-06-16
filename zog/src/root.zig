const std = @import("std");
const testing = std.testing;

const TokenizerError = error{
    Syntax,
};
const Token = struct {
    text: []u8,
    kind: enum { none, eof, id, char },

    pub export fn size(self: Token) usize {
        return self.text.len;
    }
};

const Tokenizer = struct {
    all: []u8,
    cursor: usize,
    current: Token,

    pub export fn of(all: []u8) Tokenizer {
        return .{ .all = all, .current = Token.none, .cursor = 0, .text = "" };
    }

    pub export fn next() void {
        return;
    }

    pub export fn peek() TokenizerError!Token {
        return;
    }
};
