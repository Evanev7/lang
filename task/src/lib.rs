#![allow(unused)]
pub mod legacy;

pub mod plan;
pub mod strategy;

pub enum GraphBuildErr {
    CycleDetected,
}

#[derive(Debug)]
pub enum Error {}
