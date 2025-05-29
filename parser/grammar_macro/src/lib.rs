#![allow(unused)]
/*
    we're gonna handwrite this one.
    grammar! grammar (lmao):
    grammar! {
        Grammar = Line+
        Line = AndLine, OrLine
        AndLine = Identifier '=' IdentifierOrLiteral+
        OrLine = Identifier '=' IdentifierOrLiteral OrLinePart*
        OrLinePart = ',' IdentifierOrLiteral
        IdentifierOrLiteral = Identifier, Literal
        Literal = '\'' String '\''
        Identifier = String
    }
    plus some constants
    {
        NonZeroDigit = 1,2,3,4,5,6,7,8,9
        Digit = 0, NonZeroDigit
        Number = NonZeroDigit Digit*
        LowercaseChar = 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
        UppercaseChar = 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
        Char = LowercaseChar, UppercaseChar
        String = Char*
    }

    We want to expan this into the following code:

    struct Grammar(Vec<Line>)
    enum Line(AndLine, OrLine)
    struct AndLine(Identifier, Vec<IdentifierOrLiteral>)
    struct OrLine(Identifier, IdentifierOrLiteral, Vec<OrLinePart>)
    struct OrLinePart(Box<IdentifierOrLiteral>)
    enum IdentifierOrLiteral(Identifier, Literal)
    struct Identifier(String);
    struct Literal(String)
    # and constants
    enum NonZeroDigit(D1,D2,D3,D4,D5,D6,D7,D8,D9)
    enum Digit(D0, NonZeroDigit)
    enum Number(NonZeroDigit, Vec<Digit>)
    enum LowercaseChar(La,Lb,Lc,Ld,Le,...,Lz)
    enum UppercaseChar(LA,LB,...,LZ)
    enum Char(LowercaseChar, UppercaseChar)
    struct String(Vec<Char>)

    in this case we don't expand our own num/char types, since we're generating rust code we can just use syn primitives.

*/

use proc_macro2::TokenStream as TokenStream2;
use quote::{quote, ToTokens};
use syn::{
    parse::{Parse, ParseStream},
    Token,
};

fn parse_some<T: Parse>(input: ParseStream) -> Vec<T> {
    let mut out = Vec::new();
    while let Ok(parse_result) = input.parse() {
        out.push(parse_result)
    }
    out
}

// No funky recursion here.
pub struct Grammar(Vec<Line>);
impl Parse for Grammar {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let out = parse_some(input);
        if out.is_empty() {
            // This can never hit the Ok() case, as otherwise parse_some would have caught the value.
            return input.parse::<Line>().map(|f| Self(vec![f]));
        }
        Ok(Self(out))
    }
}
pub enum Line {
    AndLine(AndLine),
    OrLine(OrLine),
}
pub struct AndLine(Identifier, Vec<IdentifierOrLiteral>);
impl Parse for AndLine {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let ident = input.parse()?;
        _ = input.parse::<Token![=]>()?;
        Ok(Self(ident, parse_some(input)))
    }
}
pub struct OrLine(Identifier, IdentifierOrLiteral, Vec<LinePart>);
impl Parse for OrLine {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let ident = input.parse()?;
        _ = input.parse::<Token![=]>()?;
        let first_def = input.parse()?;
        let remaining_defs = parse_some(input);
        Ok(Self(ident, first_def, remaining_defs))
    }
}
impl quote::ToTokens for OrLine {
    fn to_tokens(&self, tokens: &mut TokenStream2) {
        let Self(identifier, first_part, other_parts) = self;
        tokens.extend(quote! {
            pub enum #identifier {
                #first_part,
                #(#other_parts,)*
            }
        });
    }
}
pub struct LinePart(IdentifierOrLiteral);
impl Parse for LinePart {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        _ = input.parse::<Token![,]>()?;
        input.parse().map(LinePart)
    }
}
impl quote::ToTokens for LinePart {
    fn to_tokens(&self, tokens: &mut TokenStream2) {}
}
pub enum IdentifierOrLiteral {
    Identifier(Identifier),
    Literal(Literal),
}
impl Parse for IdentifierOrLiteral {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        if input.peek(Literal(syn::Lit)) {
            return Ok(IdentifierOrLiteral::Literal(literal));
        } else {
            input
                .parse::<Identifier>()
                .map(IdentifierOrLiteral::Identifier)
        }
    }
}
impl ToTokens for IdentifierOrLiteral {}
pub struct Identifier(syn::Ident);
impl Parse for Identifier {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        input.parse().map(Self)
    }
}
impl ToTokens for Identifier {
    fn to_tokens(&self, tokens: &mut TokenStream2) {
        let inner = &self.0;
        tokens.extend(quote! {#inner});
    }
}
pub struct Literal(syn::Lit);
impl Parse for Literal {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        input.parse().map(Self)
    }
}
impl quote::ToTokens for Literal {
    fn to_tokens(&self, tokens: &mut TokenStream2) {
        let inner = &self.0;
        tokens.extend(quote! {
            #inner
        });
    }
}
