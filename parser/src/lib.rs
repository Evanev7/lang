use do_notation::{m, Lift};
use std::marker::PhantomData;

macro_rules! highlight {
    ($($tt:tt)*) => {};
}
// start simple moron
// a parser is a function type.

pub struct Parser<T>(Box<dyn for<'a> Fn(&'a str) -> Vec<(T, &'a str)>>);
impl<T: 'static> Parser<T> {
    pub fn munch<'a>(&self, source: &'a str) -> Vec<(T, &'a str)> {
        (self.0)(source)
    }
    pub fn map<U>(self: Parser<T>, op: impl Fn(T) -> U + 'static) -> Parser<U> {
        Parser(Box::new(move |source| {
            self.munch(source)
                .into_iter()
                .map(|(o, i)| (op(o), i))
                .collect()
        }))
    }
    pub fn and_then<U: 'static>(
        self: Parser<T>,
        op: impl Fn(T) -> Parser<U> + 'static,
    ) -> Parser<U> {
        self.map(op).flatten()
    }
}
impl<T: 'static> Parser<Parser<T>> {
    pub fn flatten(self) -> Parser<T> {
        Parser(Box::new(move |source| {
            self.munch(source)
                .into_iter()
                .flat_map(|(o, i)| o.munch(i))
                .collect()
        }))
    }
}

impl<T: 'static + Clone> Lift<T> for Parser<T> {
    fn lift(a: T) -> Self {
        Self(Box::new(move |source| vec![(a.clone(), source)]))
    }
}

pub fn zero<T>() -> Parser<T> {
    Parser(Box::new(move |_| vec![]))
}

pub fn guard(if_only: bool) -> Parser<char> {
    if if_only {
        Lift::lift(' ')
    } else {
        zero()
    }
}

pub fn item() -> Parser<char> {
    Parser(Box::new(move |source| match source.chars().next() {
        Some(c) => vec![(c, &source[c.len_utf8()..])],
        None => vec![],
    }))
}

pub fn sat(p: impl Fn(char) -> bool + 'static) -> Parser<char> {
    m! {
        x <- item();
        guard(p(x));
        return x;
    }
}

pub fn parse_string<'a>(s: String) -> Parser<String> {
    match s.chars().next() {
        None => Lift::lift("".to_owned()),
        Some(c) => (sat(move |it| it == c)).and_then(move |_| {
            (parse_string(s[c.len_utf8()..].to_owned())).and_then({
                let value = s.clone();
                move |_| do_notation::Lift::lift(value)
            })
        }),
    }
}
