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

#[derive(Debug)]
pub struct Token<'a> {
    pub kind: TokenType,
    pub source: &'a str,
    //pub line: u32,
    //pub column: u32,
}

pub fn next<'a>(source: &'a str) -> Option<(Token<'a>, &'a str)> {
    use TokenType::*;
    let mut iter = source.char_indices().peekable();

    iter.next().and_then(|(i, c)| match c {
        '{' => Some(LCurly.with(source, i..i + 1)),
        '}' => Some(RCurly.with(source, i..i + 1)),
        '(' => Some(LParen.with(source, i..i + 1)),
        ')' => Some(RParen.with(source, i..i + 1)),
        ':' => Some(Colon.with(source, i..i + 1)),
        '0'..='9' => {
            let mut ct = 0;
            while let Some(_) = iter.next_if(|(_i, c)| {
                ct += 1;
                matches!(c, '0'..='9')
            }) {}
            Some(LitNum.with(source, i..i + ct))
        }
        'a'..='z' | 'A'..='Z' => {
            let mut ct = 0;
            while let Some(_) = iter.next_if(|(_i, c)| {
                ct += 1;
                matches!(c, 'a'..='z' | 'A'..='Z' | '_')
            }) {}
            Some(LitNum.with(source, i..i + ct))
        }
        _ => None,
    })
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
    fn with<'a>(self, source: &'a str, range: std::ops::Range<usize>) -> (Token<'a>, &'a str) {
        (self.on(&source[range.clone()]), &source[range.end..])
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
