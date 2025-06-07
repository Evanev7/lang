#[allow(unused)]

macro_rules! highlight {
    ($($tt:tt)*) => {};
}

pub struct Token<'a> {
    ttype: u8,
    start: u64,
    view: &'a str,
}

#[cfg(test)]
mod test {
    #[test]
    fn test() {
        let foo = "";
        //foo.len();
    }
}
highlight! {
tokens! {
    
}
}
