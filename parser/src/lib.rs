use std::marker;

pub type ParseResult<'a, I, O> = Vec<(O, &'a [I])>;

pub enum EitherParser<T: Parser, U: Parser<I = T::I, O = T::O>> {
    Left(T),
    Right(U),
}
impl<T: Parser, U: Parser<I = T::I, O = T::O>> Parser for EitherParser<T, U> {
    type I = T::I;
    type O = T::O;
    fn munch<'a>(&mut self, source: &'a [Self::I]) -> ParseResult<'a, Self::I, Self::O> {
        match self {
            Self::Left(p) => p.munch(source),
            Self::Right(p) => p.munch(source),
        }
    }
}

pub struct Bind<T, F> {
    inner: T,
    map: F,
}
impl<O: Parser<I = T::I>, T: Parser, F: Fn(T::O) -> O> Parser for Bind<T, F> {
    type I = T::I;
    type O = O::O;
    fn munch<'a>(&mut self, source: &'a [Self::I]) -> ParseResult<'a, Self::I, Self::O> {
        self.inner
            .munch(source)
            .into_iter()
            .flat_map(|(i, o)| (self.map)(i).munch(o).into_iter())
            .collect()
    }
}
pub struct Zero<I, O>(marker::PhantomData<I>, marker::PhantomData<O>);
impl<I, O> Parser for Zero<I, O> {
    type I = I;
    type O = O;
    fn munch<'a>(&mut self, _source: &'a [Self::I]) -> ParseResult<'a, Self::I, Self::O> {
        ParseResult::<'a, Self::I, Self::O>::new()
    }
}
pub struct Item<I> {
    _marker: marker::PhantomData<I>,
}
impl<I: Clone> Parser for Item<I> {
    type I = I;
    type O = I;
    fn munch<'a>(&mut self, source: &'a [Self::I]) -> ParseResult<'a, Self::I, Self::O> {
        vec![(source[0].clone(), &source[1..])]
    }
}

pub fn sat<I: Clone>(
    sat: impl Fn(I) -> bool,
) -> Bind<Item<I>, impl Fn(I) -> EitherParser<Raise<I, I>, Zero<I, I>>> {
    Item {
        _marker: marker::PhantomData,
    }
    .bind::<Item<I>, _>(move |item: I| {
        if sat(item.clone()) {
            EitherParser::Left(Raise {
                val: item,
                _marker: marker::PhantomData,
            })
        } else {
            EitherParser::Right(Zero(marker::PhantomData, marker::PhantomData))
        }
    })
}
pub struct Raise<I, O> {
    val: O,
    _marker: marker::PhantomData<I>,
}
impl<I, O: Clone> Parser for Raise<I, O> {
    type I = I;
    type O = O;
    fn munch<'a>(&mut self, source: &'a [Self::I]) -> ParseResult<'a, Self::I, Self::O> {
        vec![(self.val.clone(), source)]
    }
}

pub trait Parser {
    type I;
    type O;
    fn bind<T: Parser, F>(self, map: F) -> Bind<Self, F>
    where
        Self: Sized,
    {
        Bind { inner: self, map }
    }
    fn munch<'a>(&mut self, source: &'a [Self::I]) -> ParseResult<'a, Self::I, Self::O>;
}

#[cfg(test)]
mod test {
    use crate::{sat, Parser};

    #[test]
    fn test() {
        let x = 14;
        let mut letter = sat(|it: u8| it < x);
        dbg!(letter.munch(&[12, 139, 149]));
        dbg!(letter.munch(&[139, 149]));
        dbg!(std::mem::size_of_val(&letter));
    }
}
