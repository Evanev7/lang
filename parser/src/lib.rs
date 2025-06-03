use std::iter;
use std::marker;

pub trait Parser {
    type I;
    type O;
    type It: Iterator<Item = (Self::O, Self::I)>;
    fn bind<F>(self, map: F) -> Bind<Self, F>
    where
        Self: Sized,
    {
        Bind { inner: self, map }
    }
    fn munch(&mut self, source: Self::I) -> Self::It;
    fn or<T>(self, parser: T) -> Or<Self, T>
    where
        Self: Sized,
    {
        Or {
            first: Some(self),
            second: parser,
        }
    }
}

// The 'strength' comes from Monad Comprehension Syntax:
// [foo] = Raise { val: foo }
// sat p = [x | x <- item, p(x)] = item.bind(|x| -> if p(x) { result(x) } else { zero })

pub struct Or<A, B> {
    first: Option<A>,
    second: B,
}
impl<A: Parser, B: Parser<I = A::I, O = A::O, It = A::It>> Parser for Or<A, B> {
    type I = A::I;
    type O = A::O;
    type It = A::It;
    fn munch(&mut self, source: Self::I) -> Self::It {
        match self.first.as_mut() {
            Some(ref mut p) => p.munch(source),
            None => self.second.munch(source),
        }
    }
}

pub enum EitherParser<T: Parser, U: Parser<I = T::I, O = T::O>> {
    Left(T),
    Right(U),
}
impl<T: Parser, U: Parser<I = T::I, O = T::O, It = T::It>> Parser for EitherParser<T, U> {
    type I = T::I;
    type O = T::O;
    type It = T::It;
    fn munch(&mut self, source: Self::I) -> Self::It {
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
    type It = _;
    fn munch(&mut self, source: Self::I) -> Self::It {
        BindIter {
            iter: self.inner.munch(source),
        } // Iter<Item=(a,b)> where f(a): Parser<I = b>. we want to return an iterator over elements a.munch(b)
    }
}

pub struct BindIter<T> {
    iter: T,
}
impl<T> Iterator for BindIter<T>
where
    T: Iterator<Item = (A, B)>,
    A: Parser,
{
    type Item = _;
    fn next(&mut self) -> Option<Self::Item> {}
}

pub struct Zero<I, O>(marker::PhantomData<I>, marker::PhantomData<O>);
impl<I, O> Parser for Zero<I, O> {
    type I = I;
    type O = O;
    type It = iter::Empty<(O, I)>;
    fn munch(&mut self, _source: Self::I) -> Self::It {
        //Vec::<(Self::O, &'a [Self::I])>::new().into_iter()
        iter::empty()
    }
}
pub struct Item<I> {
    _marker: marker::PhantomData<I>,
}
impl<I: Iterator> Parser for Item<I> {
    type I = I;
    type O = I::Item;
    type It = std::option::IntoIter<(I::Item, I)>;
    fn munch(&mut self, mut source: Self::I) -> Self::It {
        source.next().map(move |it| (it, source)).into_iter()
    }
}

pub fn sat<I>(
    sat_fn: impl Fn(I::Item) -> bool,
) -> Bind<Item<I>, impl Fn(I::Item) -> EitherParser<Raise<I, I::Item>, Zero<I, I::Item>>>
where
    I: Iterator,
    I::Item: Clone,
{
    Item {
        _marker: marker::PhantomData,
    }
    .bind(move |item: I::Item| {
        if sat_fn(item.clone()) {
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
    type It = iter::Once<(O, I)>;
    fn munch<'a>(&mut self, source: Self::I) -> Self::It {
        iter::once((self.val.clone(), source))
    }
}

#[cfg(test)]
mod test {
    use crate::*;
    use std::array::IntoIter;

    #[test]
    fn test() {
        let x = 14;
        let mut letter = sat::<IntoIter<u8, 3>>(|it: u8| it < x);
        dbg!(letter.munch([12, 139, 149].into_iter()));
        dbg!(letter.munch([139, 149].into_iter()));
        dbg!(std::mem::size_of_val(&letter));
    }
}
