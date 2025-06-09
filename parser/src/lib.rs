#[allow(unused)]
macro_rules! highlight {
    ($($tt:tt)*) => {};
}

highlight! {
    fn main() { x = 1, y = 2, print(x) }
}

pub enum ParseError {
    Null,
}

#[derive(Debug, PartialEq)]
pub struct Token<'a> {
    pub kind: TokenType,
    pub source: &'a str,
    //pub line: u32,
    //pub column: u32,
}

struct Program<'a> {
    function: Function<'a>,
}
struct Function<'a> {
    name: Token<'a>,
    body: FunctionBody<'a>,
}
struct FunctionBody<'a> {
    expr: Expression<'a>,
}
struct Expression<'a> {
    value: u32,
    marker: std::marker::PhantomData<&'a u32>,
}
// To start with, our grammar is:
highlight! {
    Program = Function
    Function = KFn LitId LParen RParen LCurly Statement RCurly
    Statement = LetStatement, FinalStatement
    LetStatement = KLet LitId Colon LitId Equals LitNum Comma
    FinalStatement = LitId, LitNum, Statement
}

pub fn parse<'a>(toks: TokenStream<'a>) {
    highlight!(
        data = parse_data(toks);
        functions = parse_functions(toks);
    );
}

// Lazy tokens
pub struct TokenStream<'a>(std::str::Chars<'a>);

impl<'a> Iterator for TokenStream<'a> {
    type Item = Token<'a>;
    fn next(&mut self) -> Option<Self::Item> {
        use TokenType::*;
        let source = self.0.as_str();
        self.0.next().map(|c| {
            match c {
                '{' => LCurly.on(&source[..1]),
                '}' => RCurly.on(&source[..1]),
                '(' => LParen.on(&source[..1]),
                ')' => RParen.on(&source[..1]),
                ':' => Colon.on(&source[..1]),
                '0'..='9' => {
                    let mut peekable = self.0.clone().peekable();
                    let mut ct = 1;
                    // Match [0-9]*
                    while peekable.next_if(|c| matches!(c, '0'..='9' | '_')).is_some() {
                        self.0.next();
                        ct += 1;
                    }
                    // Match .?
                    if let Some('.') = peekable.peek() {
                        ct += 1;
                        peekable.next();
                        self.0.next();
                    }
                    while peekable.next_if(|c| matches!(c, '0'..='9' | '_')).is_some() {
                        ct += 1;
                        self.0.next();
                    }
                    LitNum.on(&source[..ct])
                }
                '"' => {
                    let mut peekable = self.0.clone().peekable();
                    let mut ct = 1;
                    while peekable.next_if(|c| !matches!(c, '"')).is_some() {
                        ct += 1;
                        self.0.next();
                    }
                    if peekable.next_if(|it| matches!(it, '"')).is_some() {
                        self.0.next();
                        ct += 1;
                        LitStr.on(&source[..ct])
                    } else {
                        // Perhaps UnfinishedStr? Perhaps early terminate on }?
                        Unknown.on(&source[..ct])
                    }
                }
                'a'..='z' | 'A'..='Z' => {
                    let mut peekable = self.0.clone().peekable();
                    let mut ct = 1;
                    while peekable
                        .next_if(|c| matches!(c, 'a'..='z' | 'A'..='Z' | '_'))
                        .is_some()
                    {
                        self.0.next();
                        ct += 1;
                    }
                    match &source[..ct] {
                        "fn" => KFn.on(&source[..ct]),
                        "let" => KLet.on(&source[..ct]),
                        "return" => KRet.on(&source[..ct]),
                        _ => LitId.on(&source[..ct]),
                    }
                }
                ',' | '\n' | ' ' | '\t' | '\r' => {
                    let mut peekable = self.0.clone().peekable();
                    let mut contains_comma = matches!(c, ',' | '\n');
                    let mut ct = 1;
                    while let Some(_) = peekable.next_if(|c| matches!(c, ' ' | '\t' | '\r')) {
                        ct += 1;
                        self.0.next();
                        contains_comma |= matches!(c, ',' | '\n');
                    }
                    if contains_comma {
                        Comma.on(&source[..ct])
                    } else {
                        Sep.on(&source[..ct])
                    }
                }
                _ => Unknown.on(&source[..1]),
            }
        })
    }
}

impl<'a> TokenStream<'a> {
    pub fn new(source: &'a str) -> Self {
        Self(source.chars())
    }
}

pub fn lex<'a>(source: &'a str) -> Vec<Token<'a>> {
    TokenStream::new(source).collect()
    /*use TokenType::*;
    let mut iter = source.char_indices().peekable();
    let mut out = vec![];
    while let Some((i, c)) = iter.next() {
        out.push(match c {
            '{' => LCurly.on(&source[i..i + 1]),
            '}' => RCurly.on(&source[i..i + 1]),
            '(' => LParen.on(&source[i..i + 1]),
            ')' => RParen.on(&source[i..i + 1]),
            ':' => Colon.on(&source[i..i + 1]),
            '0'..='9' => {
                let mut ct = 1;
                // Match [0-9]*
                while let Some(_) = iter.next_if(|(_i, c)| matches!(c, '0'..='9' | '_')) {
                    ct += 1;
                }
                // Match .?
                if let Some((_i, '.')) = iter.peek() {
                    ct += 1;
                    iter.next();
                }
                while let Some(_) = iter.next_if(|(_i, c)| matches!(c, '0'..='9' | '_')) {
                    ct += 1;
                }
                LitNum.on(&source[i..i + ct])
            }
            '"' => {
                let mut ct = 1;
                while iter.next_if(|(_i, c)| !matches!(c, '"')).is_some() {
                    ct += 1
                }
                if let Some((_i, '"')) = iter.peek() {
                    iter.next();
                    ct += 1;
                    LitStr.on(&source[i..i + ct])
                } else {
                    Unknown.on(&source[i..i + ct])
                }
            }
            'a'..='z' | 'A'..='Z' => {
                let mut ct = 1;
                while iter
                    .next_if(|(_i, c)| matches!(c, 'a'..='z' | 'A'..='Z' | '_'))
                    .is_some()
                {
                    ct += 1;
                }
                match &source[i..i + ct] {
                    "fn" => KFn.on(&source[i..i + ct]),
                    "let" => KLet.on(&source[i..i + ct]),
                    "return" => KRet.on(&source[i..i + ct]),
                    _ => LitId.on(&source[i..i + ct]),
                }
            }
            ',' | '\n' | ' ' | '\t' | '\r' => {
                let mut contains_comma = matches!(c, ',' | '\n');
                let mut ct = 1;
                while let Some(_) = iter.next_if(|(_i, c)| matches!(c, ' ' | '\t' | '\r')) {
                    ct += 1;
                    contains_comma |= matches!(c, ',' | '\n');
                }
                if contains_comma {
                    Comma.on(&source[i..i + ct])
                } else {
                    Sep.on(&source[i..i + ct])
                }
            }
            _ => Unknown.on(&source[i..i + 1]),
        });
    }
    out.push(EOF.on(&""));
    out*/
}

#[derive(Debug, PartialEq)]
#[non_exhaustive]
pub enum TokenType {
    LCurly,
    RCurly,
    LParen,
    RParen,
    Colon,

    Sep,
    Comma,

    LitNum,
    LitId,
    LitStr,

    KLet,
    KRet,
    KFn,
    KMatch,

    SymEq,
    SymAdd,
    SymSub,

    Unknown,
    EOF,
}

impl TokenType {
    fn on<'a>(self, source: &'a str) -> Token<'a> {
        Token { kind: self, source }
    }
}

#[cfg(test)]
mod test {
    use super::*;
    #[test]
    fn test() {
        dbg!(TokenStream::new("{ }").collect::<Vec<_>>());
        dbg!(TokenStream::new(
            "fn main() {
            print(23.14)
        }
        "
        )
        .collect::<Vec<_>>());
    }
    #[test]
    fn int_lits() {
        assert_eq!(
            lex("12"),
            vec![TokenType::LitNum.on("12"), TokenType::EOF.on("")]
        );
        assert_eq!(
            lex("23.14"),
            vec![TokenType::LitNum.on("23.14"), TokenType::EOF.on("")]
        );
        assert_eq!(
            lex("23."),
            vec![TokenType::LitNum.on("23."), TokenType::EOF.on("")]
        );
        // I think we're not using .12
        assert_eq!(
            lex(".12"),
            vec![
                TokenType::Unknown.on("."),
                TokenType::LitNum.on("12"),
                TokenType::EOF.on("")
            ]
        );
        assert_eq!(
            lex("."),
            vec![TokenType::Unknown.on("."), TokenType::EOF.on("")]
        );
    }
}

highlight! {
    task IntegrateMotion {
        input {
            positions: [Vec3]
            velocity: [Vec3]
            forces: [Vec3]
            coe: [float]
        }
        params {
            dt: float
        }

        stage detect_collisions(&positions, dt) -> collision_pairs
        stage effects_collisions(..) -> ..

        flow {

        }
    }
}
