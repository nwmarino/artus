# source paths

# io.s (std)
-> ('io', ('std', null))

fn sys {
    ...
}

# foo.s
-> ('foo', ('helpers', null))

import std::io 

fn stuff {
    call sys
}

# util.s
-> ('util', null)

import helpers::foo

fn util {
    call stuff
}

# main.s
-> ('main', null)

import util

fn main {
    call util
}

# source tree

src/
|-main.s
|-util.s
`-helpers/
   `-foo.s
