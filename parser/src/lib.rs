#[allow(unused)]
macro_rules! highlight {
    ($($tt:tt)*) => {};
}

highlight! {
    fn main() { x = 1, y = 2, print(x) }
}

#[derive(Debug)]
pub struct Token<'a> {
    pub kind: TokenType,
    pub source: &'a str,
    //pub line: u32,
    //pub column: u32,
}

impl<'a> Token<'a> {
    pub fn single(source: &'a str) -> Option<Token<'a>> {
        use TokenType::*;

        match source.chars().next()? {
            '{' => Some(LCurly),
            '}' => Some(RCurly),
            '(' => Some(LParen),
            ')' => Some(RParen),
            ':' => Some(Colon),
            _ => None,
        }
        .map(|kind| Token {
            kind,
            source: &source[..1],
        })
    }
    pub fn multi(source: &'a str) -> Option<Token<'a>> {
        use TokenType::*;

        match source {
            it if it.starts_with("let") => return Some(KWLet.on(&source[..3])),
            it if it.starts_with("fn") => return Some(KWFunc.on(&source[..2])),
            it if it.starts_with("return") => return Some(KWRet.on(&source[..6])),
            _ => {}
        };
        let mut iter = source.char_indices().peekable();
        let mut lock = None;
        while let Some((_i, c)) = iter.peek() {
            match c {
                '0'..='9' if lock == None => lock = Some(LitNum),
                '0'..='9' if lock == Some(LitNum) => {}
                'a'..='z' if lock == None => lock = Some(LitId),
                'a'..='z' if lock == Some(LitId) => {}
                'A'..='Z' if lock == None => lock = Some(LitId),
                'A'..='Z' if lock == Some(LitId) => {}
                '_' if lock == Some(LitId) => {}
                ' ' if lock == None => lock = Some(Space),
                ' ' if lock == Some(Space) => {}
                _ => break,
            }
            iter.next();
        }
        iter.next()
            .zip(lock)
            .map(|((i, _c), tt)| tt.on(&source[..i]))
    }
    pub fn next(source: &'a str) -> Option<Token<'a>> {
        Self::single(source).or_else(|| Self::multi(source))
    }
}

pub fn parse<'a>(source: &'a str) -> Vec<Token<'a>> {
    vec![]
}

#[derive(Debug, PartialEq)]
#[non_exhaustive]
pub enum TokenType {
    LCurly,
    RCurly,
    LParen,
    RParen,
    Space,
    Comma,
    Colon,
    LitNum,
    LitId,
    KWLet,
    KWRet,
    KWFunc,
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
        dbg!("hiiii");
        dbg!(parse("{ }"));
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
