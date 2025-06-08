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

pub fn lex<'a>(source: &'a str) -> Vec<Token<'a>> {
    use TokenType::*;
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
                while let Some(_) = iter.next_if(|(_i, c)| matches!(c, '0'..='9')) {
                    ct += 1;
                }
                // Match .?
                if let Some((_i, '.')) = iter.peek() {
                    ct += 1;
                    iter.next();
                }
                while let Some(_) = iter.next_if(|(_i, c)| matches!(c, '0'..='9')) {
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
                    "fn" => KWFunc.on(&source[i..i + ct]),
                    "let" => KWLet.on(&source[i..i + ct]),
                    "return" => KWRet.on(&source[i..i + ct]),
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
    out
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

    KWLet,
    KWRet,
    KWFunc,

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
        dbg!(lex("{ }"));
        dbg!(lex("
        fn main() {
            print(23.14)
        }
        "));
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
