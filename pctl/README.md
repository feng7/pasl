% The Parallel-Container Template Library User's Guide
% Deepsea Project
% 10 February 2015

Introduction
============

The purpose of this document is to serve as a working draft of the
design and implementation of the Parallel Container Template Library
(PCTL).

***Essential terminology.***

A *function call operator* (or, just "call operator") is a member
function of a C++ class that is specified by the name `operator()`.

A *functor* is a C++ class which defines a call operator.

A *right-open range* is ...

*work* *span* 

Containers
==========

Sequence containers
-------------------

Class name                           | Description
-------------------------------------|---------------------------------
[`parray`](#parray)                  | Array class
[`pchunkedseq`](#pchunkedseq)        | Parallel chunked sequence class

Table: Sequence containers that are provided by pctl.

Associative containers
----------------------

Class name          | Description
--------------------|-------------------------
set                 | Set class
map                 | Associative map class

Table: Associative containers that are provided by pctl.

Parallel array {#parray}
==============

+-----------------------------------+-----------------------------------+
| Template parameter                | Description                       |
+===================================+===================================+
| [`Item`](#pa-item)                | Type of the objects to be stored  |
|                                   |in the container                   |
+-----------------------------------+-----------------------------------+
| [`Alloc`](#pa-alloc)              | Allocator to be used by the       |
|                                   |container to construct and destruct|
|                                   |objects of type `Item`             |
+-----------------------------------+-----------------------------------+

Table: Template parameters for the `parray` class.
                                                           
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace parray {

template <class Item, class Alloc = std::allocator<Item>>
class parray;

} } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------------------------+-----------------------------------+
| Type                              | Description                       |
+===================================+===================================+
| `value_type`                      | Alias for template parameter      |
|                                   |`Item`                             |
+-----------------------------------+-----------------------------------+
| `reference`                       | Alias for `value_type&`           |
+-----------------------------------+-----------------------------------+
| `const_reference`                 | Alias for `const value_type&`     |
+-----------------------------------+-----------------------------------+
| `pointer`                         | Alias for `value_type*`           |
+-----------------------------------+-----------------------------------+
| `const_pointer`                   | Alias for `const value_type*`     |
+-----------------------------------+-----------------------------------+
| [`iterator`](#pa-iter)            | Iterator                          |
+-----------------------------------+-----------------------------------+
| [`const_iterator`](#pa-iter)      | Const iterator                    |
+-----------------------------------+-----------------------------------+

Table: Parallel-array type definitions.

+-----------------------------------+-----------------------------------+
| Constructor                       | Description                       |
+===================================+===================================+
| [empty container                  | constructs an empty container with|
|constructor](#pa-e-c-c) (default   |no items                           |
|constructor)                       |                                   |
+-----------------------------------+-----------------------------------+
| [fill constructor](#pa-e-f-c)     | constructs a container with a     |
|                                   |specified number of copies of a    |
|                                   |given item                         |
+-----------------------------------+-----------------------------------+
| [populate constructor](#pa-e-p-c) | constructs a container with a     |
|                                   |specified number of values that are|
|                                   |computed by a specified function   |
+-----------------------------------+-----------------------------------+
| [copy constructor](#pa-e-cp-c)    | constructs a container with a copy|
|                                   |of each of the items in the given  |
|                                   |container, in the same order       |
+-----------------------------------+-----------------------------------+
| [initializer list](#pa-i-l-c)     | constructs a container with the   |
|                                   |items specified in a given         |
|                                   |initializer list                   |
+-----------------------------------+-----------------------------------+
| [move constructor](#pa-m-c)       | constructs a container that       |
|                                   |acquires the items of a given      |
|                                   |parallel array                     |
+-----------------------------------+-----------------------------------+
| [destructor](#pa-destr)           | destructs a container             |
+-----------------------------------+-----------------------------------+

Table: Parallel-array constructors and destructors.

## Template parameters

### Item type {#pa-item}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Item;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type of the items to be stored in the container.

Objects of type `Item` should be default constructable.

### Allocator {#pa-alloc}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Alloc;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Allocator class.

## Iterator {#pa-iter}

The type `iterator` and `const_iterator` are instances of the
[random-access
iterator](http://en.cppreference.com/w/cpp/concept/RandomAccessIterator)
concept.

## Constructors and destructors

### Empty container constructor {#pa-e-c-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
parray();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

***Complexity.*** Constant time.

Constructs an empty container with no items;

### Fill container {#pa-e-f-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
parray(long n, const value_type& val);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with `n` copies of `val`.

***Complexity.*** Work and span are linear and logarithmic in the size
   of the resulting container, respectively.

### Populate constructor {#pa-e-p-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
// (1) Constant-time body
parray(long n, std::function<Item(long)> body);
// (2) Non-constant-time body
parray(long n,
       std::function<long(long)> body_comp,
       std::function<Item(long)> body);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with `n` cells, populating those cells with
values returned by the `n` calls, `body(0)`, `body(1)`, ...,
`body(n-1)`, in that order.

In the second version, the value returned by `body_comp(i)` is used by
the constructor as the complexity estimate for the call `body(i)`.

***Complexity.*** TODO

### Copy constructor {#pa-e-cp-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
parray(const parray& other);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with a copy of each of the items in `other`, in
the same order.

***Complexity.*** Work and span are linear and logarithmic in the size
   of the resulting container, respectively.

### Initializer-list constructor {#pa-i-l-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
parray(initializer_list<value_type> il);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with the items in `il`.

***Complexity.*** Work and span are linear in the size of the resulting
   container.

### Move constructor {#pa-m-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
parray(parray&& x);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container that acquires the items of `other`.

***Complexity.*** Constant time.

### Destructor {#pa-destr}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
~parray();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Destructs the container.

***Complexity.*** Work and span are linear and logarithmic in the size
   of the container, respectively.

## Operations

+------------------------+--------------------------------------+
| Operation              | Description                          |
+========================+======================================+
| [`operator[]`](#pa-i-o)| Access member item                   |
|                        |                                      |
+------------------------+--------------------------------------+
| [`size`](#pa-si)       | Return size                          |
+------------------------+--------------------------------------+
| [`resize`](#pa-rsz)    | Change size                          |
+------------------------+--------------------------------------+
| [`swap`](#pa-sw)       | Exchange contents                    |
+------------------------+--------------------------------------+
| [`begin`](#pa-beg)     | Returns an iterator to the beginning |
| [`cbegin`](#pa-beg)    |                                      |
+------------------------+--------------------------------------+
| [`end`](#pa-end)       | Returns an iterator to the end       |
| [`cend`](#pa-end)      |                                      |
+------------------------+--------------------------------------+

Table: Parallel-array member functions.

### Indexing operator {#pa-i-o}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
reference operator[](long i);
const_reference operator[](long i) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns a reference at the specified location `i`. No bounds check is
performed.

***Complexity.*** Constant time.

### Size operator {#pa-si}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long size() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns the size of the container.

***Complexity.*** Constant time.

### Resize {#pa-rsz}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void resize(long n, const value_type& val);
void resize(long n) {
  value_type val;
  resize(n, val);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Resizes the container so that it contains `n` items.

The contents of the current container are removed and replaced by `n`
copies of the item referenced by `val`.

***Complexity.*** Let $m$ be the size of the container just before and
   $n$ just after the resize operation. Then, the work and span are
   linear and logarithmic in $\max(m, n)$, respectively.

### Exchange operation {#pa-sw}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void swap(parray& other);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Exchanges the contents of the container with those of `other`. Does
not invoke any move, copy, or swap operations on individual items.

***Complexity.*** Constant time.

### Iterator begin {#pa-beg}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
iterator begin() const;
const_iterator cbegin() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns an iterator to the first item of the container.

If the container is empty, the returned iterator will be equal to
end().

***Complexity.*** Constant time.

### Iterator end {#pa-end}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
iterator end() const;
const_iterator cend() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns an iterator to the element following the last item of the
container.

This element acts as a placeholder; attempting to access it results in
undefined behavior.

***Complexity.*** Constant time.

Parallel chunked sequence {#pchunkedseq}
=========================

+-----------------------------------+-----------------------------------+
| Template parameter                | Description                       |
+===================================+===================================+
| [`Item`](#cs-item)                | Type of the objects to be stored  |
|                                   |in the container                   |
+-----------------------------------+-----------------------------------+
| [`Alloc`](#cs-alloc)              | Allocator to be used by the       |
|                                   |container to construct and destruct|
|                                   |objects of type `Item`             |
+-----------------------------------+-----------------------------------+

Table: Template parameters for the `pchunkedseq` class.
                                                           
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace parray {

template <class Item, class Alloc = std::allocator<Item>>
class pchunkedseq;

} } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------------------------+-----------------------------------+
| Type                              | Description                       |
+===================================+===================================+
| `value_type`                      | Alias for template parameter      |
|                                   |`Item`                             |
+-----------------------------------+-----------------------------------+
| `reference`                       | Alias for `value_type&`           |
+-----------------------------------+-----------------------------------+
| `const_reference`                 | Alias for `const value_type&`     |
+-----------------------------------+-----------------------------------+

Table: Parallel chunked sequence type definitions.

+-----------------------------------+-----------------------------------+
| Constructor                       | Description                       |
+===================================+===================================+
| [empty container                  | constructs an empty container with|
|constructor](#cs-e-c-c) (default   |no items                           |
|constructor)                       |                                   |
+-----------------------------------+-----------------------------------+
| [fill constructor](#cs-e-f-c)     | constructs a container with a     |
|                                   |specified number of copies of a    |
|                                   |given item                         |
+-----------------------------------+-----------------------------------+
| [populate constructor](#cs-e-p-c) | constructs a container with a     |
|                                   |specified number of values that are|
|                                   |computed by a specified function   |
+-----------------------------------+-----------------------------------+
| [copy constructor](#cs-e-cp-c)    | constructs a container with a copy|
|                                   |of each of the items in the given  |
|                                   |container, in the same order       |
+-----------------------------------+-----------------------------------+
| [initializer list](#cs-i-l-c)     | constructs a container with the   |
|                                   |items specified in a given         |
|                                   |initializer list                   |
+-----------------------------------+-----------------------------------+
| [move constructor](#cs-m-c)       | constructs a container that       |
|                                   |acquires the items of a given      |
|                                   |parallel array                     |
+-----------------------------------+-----------------------------------+
| [destructor](#cs-destr)           | destructs a container             |
+-----------------------------------+-----------------------------------+

Table: Parallel chunked sequence constructors and destructors.

## Template parameters

### Item type {#cs-item}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Item;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type of the items to be stored in the container.

Objects of type `Item` should be default constructable.

### Allocator {#cs-alloc}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Alloc;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Allocator class.

## Constructors and destructors

### Empty container constructor {#cs-e-c-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
pchunkedseq();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

***Complexity.*** Constant time.

Constructs an empty container with no items;

### Fill container {#cs-e-f-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
pchunkedseq(long n, const value_type& val);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with `n` copies of `val`.

***Complexity.*** Work and span are linear and logarithmic in the size
   of the resulting container, respectively.

### Populate constructor {#cs-e-p-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
// (1) Constant-time body
pchunkedseq(long n, std::function<Item(long)> body);
// (2) Non-constant-time body
pchunkedseq(long n,
            std::function<long(long)> body_comp,
            std::function<Item(long)> body);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with `n` cells, populating those cells with
values returned by the `n` calls, `body(0)`, `body(1)`, ...,
`body(n-1)`, in that order.

In the second version, the value returned by `body_comp(i)` is used by
the constructor as the complexity estimate for the call `body(i)`.

***Complexity.*** TODO

### Copy constructor {#cs-e-cp-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
pchunkedseq(const pchunkedseq& other);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with a copy of each of the items in `other`, in
the same order.

***Complexity.*** Work and span are linear and logarithmic in the size
   of the resulting container, respectively.

### Initializer-list constructor {#cs-i-l-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
pchunkedseq(initializer_list<value_type> il);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container with the items in `il`.

***Complexity.*** Work and span are linear in the size of the resulting
   container.

### Move constructor {#cs-m-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
pchunkedseq(pchunkedseq&& x);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Constructs a container that acquires the items of `other`.

***Complexity.*** Constant time.

### Destructor {#cs-destr}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
~pchunkedseq();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Destructs the container.

***Complexity.*** Work and span are linear and logarithmic in the size
   of the container, respectively.

## Sequential operations

+-----------------------------+--------------------------------------+
| Operation                   | Description                          |
+=============================+======================================+
| [`seq.operator[]`](#cs-i-o) | Access member item                   |
+-----------------------------+--------------------------------------+
| [`seq.size`](#cs-si)        | Return size                          |
+-----------------------------+--------------------------------------+
| [`seq.swap`](#cs-sw)        | Exchange contents                    |
+-----------------------------+--------------------------------------+

Table: Sequential operations of the parallel chunked sequence.

### Indexing operator {#cs-i-o}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
reference operator[](long i);
const_reference operator[](long i) const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns a reference at the specified location `i`. No bounds check is
performed.

***Complexity.*** Constant time.

### Size operator {#cs-si}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long size() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns the size of the container.

***Complexity.*** Constant time.

### Exchange operation {#cs-sw}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void swap(pchunkedseq& other);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Exchanges the contents of the container with those of `other`. Does
not invoke any move, copy, or swap operations on individual items.

***Complexity.*** Constant time.

## Parallel operations

+------------------------+--------------------------------------+
| Operation              | Description                          |
+========================+======================================+
| [`rebuild`](#cs-rbld)  | Repopulate container changing size   |
+------------------------+--------------------------------------+
| [`resize`](#cs-rsz)    | Change size                          |
+------------------------+--------------------------------------+

Table: Parallel operations of the parallel chunked sequence.

### Rebuild {#cs-rbld}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void rebuild(long n, std::function<value_type(long)> body);
void rebuild(long n,
             std::function<long(long)> body_comp,
             std::function<value_type(long)> body);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Resizes the container so that it contains `n` items.

The contents of the current container are removed and replaced by the
`n` items returned by the `n` calls, `body(0)`, `body(1)`, ...,
`body(n-1)`, in that order.

***Complexity.*** Let $m$ be the size of the container just before and
   $n$ just after the resize operation. Then, the work and span are
   linear and logarithmic in $\max(m, n)$, respectively.

### Resize {#cs-rsz}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void resize(long n, const value_type& val);
void resize(long n) {
  value_type val;
  resize(n, val);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Resizes the container so that it contains `n` items.

The contents of the current container are removed and replaced by `n`
copies of the item referenced by `val`.

***Complexity.*** Let $m$ be the size of the container just before and
   $n$ just after the resize operation. Then, the work and span are
   linear and logarithmic in $\max(m, n)$, respectively.

Data-parallel operations
========================

Indexed-based for loop
----------------------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
parray<long> xs = { 0, 0, 0, 0 };
parallel_for(0, xs.size(), [&] (long i) {
  xs[i] = i+1;
});

std::cout << "xs = " << xs << std::endl;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
xs = { 1, 2, 3, 4 }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


### Template parameters

The following table describes the template parameters that are used by
the different version of our parallel-for function.

+---------------------------------+-----------------------------------+
| Template parameter              | Description                       |
+=================================+===================================+
| [`Iter`](#lp-iter)              | Type of the iterator to be used by|
|                                 |the loop                           |
+---------------------------------+-----------------------------------+
| [`Body`](#lp-i)                 | Loop body                         |
+---------------------------------+-----------------------------------+
| [`Seq_body_rng`](#lp-s-i)       | Sequentialized version of the body|
+---------------------------------+-----------------------------------+
| [`Comp`](#lp-c)                 | Complexity function for a         |
|                                 |specified iteration                |
+---------------------------------+-----------------------------------+
| [`Comp_rng`](#lp-c-r)           | Complexity function for a         |
|                                 |specified range of iterations      |
+---------------------------------+-----------------------------------+

Table: All template parameters used by various instance of the
parallel-for loop.

#### Loop iterator {#lp-iter}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Iter;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

At a minimum, any value of type `Iter` must support the following
operations. Let `a` and `b` denote values of type `Iter` and `n` a
value of type `long`.  Then, we need the subtraction operation `a-b`,
the comparison operation `a!=b`, the addition-by-a-number-operation
`a+n`, and the increment operation `a++`.

As such, the concept of the `Iter` class bears resemblance to the
concept of the [random-access
iterator](http://en.cppreference.com/w/cpp/concept/RandomAccessIterator).
The main difference between the two is that, with the random-access
iterator, an iterable value necessarily has the ability to
dereference, whereas with our `Iter` class this feature is not used
and therefore not required.

#### Loop body {#lp-i}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Body;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void operator()(Iter i);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Sequentialized loop body {#lp-s-i}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Seq_body_rng;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void operator()(Iter lo, Iter hi);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Complexity function {#lp-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Comp;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long operator()(Iter i);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#### Range-based compelxity function {#lp-c-r}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Comp_rng;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long operator()(Iter lo, Iter hi);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Instances

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace pctl {

template <class Iter, class Body>
void parallel_for(Iter lo, Iter hi, Body body);

template <class Iter, class Body, class Comp>
void parallel_for(Iter lo, Iter hi, Comp comp, Body body);

} }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace pctl {
namespace range {

template <
  class Iter,
  class Body,
  class Comp_rng
>
void parallel_for(Iter lo,
                  Iter hi, Comp_rng comp_rng,
                  Body body);

template <
  class Iter,
  class Body,
  class Comp_rng,
  class Seq_body_rng
>
void parallel_for(Iter lo,
                  Iter hi,
                  Comp_rng comp_rng,
                  Body body,
                  Seq_body_rng seq_body_rng);

} } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Reduction
---------

+-----------------------------------+-----------------------------------+
| Abstraction layer                 | Description                       |
+===================================+===================================+
| [Level 0](#red-l-0)               | Apply a specified monoid to       |
|                                   |combine the items of a specified   |
|                                   |sequence container                 |
+-----------------------------------+-----------------------------------+
| [Level 1](#red-l-1)               | Introduces a lift operator that   |
|                                   |allows the client to combine the   |
|                                   |reduction with a specified         |
|                                   |tabulation, such that the          |
|                                   |tabulation is injected into the    |
|                                   |leaves of the reduction tree       |
+-----------------------------------+-----------------------------------+
| [Level 2](#red-l-2)               | Introduces an operator that       |
|                                   |provides a sequentialized          |
|                                   |alternative for the lift operator  |
+-----------------------------------+-----------------------------------+
| [Level 3](#red-l-3)               | Introduces a "mergeable output"   |
|                                   |type that enables                  |
|                                   |destination-passing style reduction|
+-----------------------------------+-----------------------------------+
| [Level 4](#red-l-4)               | Introduces a "splittlable input"  |
|                                   |type that abstracts from the type  |
|                                   |of the input container             |
+-----------------------------------+-----------------------------------+

Table: Abstraction layers used by pctl for reduction operators.

### Level 0 {#red-l-0}

+---------------------------------+-----------------------------------+
| Template parameter              | Description                       |
+=================================+===================================+
| [`Iter`](#r0-iter)              | Type of the iterator to be used to|
|                                 |access items in the input container|
+---------------------------------+-----------------------------------+
| [`Item`](#r0-i)                 | Type of the items in the input    |
|                                 | container                         |
+---------------------------------+-----------------------------------+
| [`Combine`](#r0-a)              | Associative combining operator    |
+---------------------------------+-----------------------------------+
| [`Weight`](#r0-w)               | Weight function (optional)        |
+---------------------------------+-----------------------------------+

Table: Shared template parameters for all level-0 reduce operations.

#### Iter {#r0-iter}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Iter;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An instance of this class must be an implementation of the
[random-access
iterator](http://en.cppreference.com/w/cpp/concept/RandomAccessIterator).

#### Item {#r0-i}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Item;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type of the items to be processed by the reduction.

#### Associative combining operator {#r0-a}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Combine;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The combining operator is a C++ functor that takes two items and
returns a single item. The call operator for the `Combine` class
should have the following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
Item operator()(const Item& x, const Item& y);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The behavior of the reduction is well defined only if the combining
operator is *associative*.

***Associativity.*** Let `f` be an object of type `Combine`. The
   operator `f` is associative if, for any `x`, `y`, and `z` that are
   values of type `Item`, the following equality holds:

`f(x, f(y, z)) == f(f(x, y), z)`

***Example: the "max" combining operator.*** The following functor is
   associative because the `std::max` function is itself associative.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Max_combine {
public:
  long operator()(long x, long y) {
    return std::max(x, y);
  }
};
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Weight function {#r0-w}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Weight;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The weight function is a C++ functor that takes a single item and
returns a non-negative "weight value" describing the size of the
item. The call operator for the weight function should have the
following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long operator()(const Item& x);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

***Example: the array-weight function.*** Let `Item` be
   `parray<long>`. Then, one valid weight function is the weight
   function that returns the size of the given array.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class PArray_weight {
public:
  long operator()(const parray<long>& xs) {
    return xs.size();
  }
};
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Examples {#r0-parray}

At this level, we have two types of reduction for parallel arrays. The
first one assumes that the combining operator takes constant time and
the second does not.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace parray {

template <class Item, class Combine>
Item reduce(const parray<Item>& xs, Item id, Combine combine);

template <
  class Iter,
  class Item,
  class Weight,
  class Combine
>
Item reduce(Iter lo,
            Iter hi,
            Item id,
            Weight weight,
            Combine combine);

} } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

***Example: taking the maximum value of an array of numbers.*** The
following code takes the maximum value of `xs` using our `Max_combine`
functor.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long max(const parray<long>& xs) {
  return reduce(xs.cbegin(), xs.cend(), LONG_MIN, Max_combine());
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Alternatively, one can use C++ lambda expressions to implement the
same algorithm.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long max(const parray<long>& xs) {
  return reduce(xs.cbegin(), xs.cend(), LONG_MIN, [&] (long x, long y) {
    return std::max(x, y);
  });
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Complexity {#r0-complexity}

There are two cases to consider for any reduction $\mathtt{reduce}(lo,
hi, id, f)$: (1) the associative combining operator $f$ takes constant
time and (2) $f$ does not.

***(1) Constant-time associative combining operator.*** The amount of
work performed by the reduction is $O(hi-lo)$ and the span is $O(\log
hi-lo)$.

***(2) Non-constant-time associative combining operator.*** We define
$\mathcal{R}$ to be the set of all function applications $f(x, y)$
that are performed in the reduction tree. Then,

- The work performed by the reduction is $O(n + \sum_{f(x, y) \in
\mathcal{R}(f, id, lo, hi)} W(f(x, y)))$.

- The span of the reduction is $O(\log n \max_{f(x, y) \in
\mathcal{R}(f, id, lo, hi)} S(f(x, y)))$.

Under certain conditions, we can use the following lemma to deduce a
more precise bound on the amount of work performed by the
reduction.

***Lemma (Work efficiency).*** For any associative combining operator
$f$ and weight function $w$, if for any $x$, $y$,

1. $w(f(x, y)) \leq w(x) + w(y)$, and
2. $W \leq c (w(x) + w(y))$, for some constant $c$,

where $W$ denotes the amount of work performed by the call $f(x, y)$,
then the amount of work performed by the reduction is $O(\log hi-lo
\sum_{lo \leq it < hi} (1 + w(*it)))$.

***Example: using a non-constant time combining operator.*** Now, let
us consider a case where the associative combining operator takes
linear time in proportion with the combined size of its two
arguments. For this example, we will consider the following max
function, which examines a given array of arrays.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long max(const parray<parray<long>>& xss) {
  parray<long> id = { LONG_MIN };
  auto weight = [&] (const parray<long>& xs) {
    return xs.size();
  };
  auto combine = [&] (const parray<long>& xs1,
                      const parray<long>& xs2) {
    parray<long> r = { std::max(max(xs1), max(xs2)) };
    return r;
  };
  parray<long> a =
    reduce(xss.cbegin(), xcc.cend(), id, weight, combine);
  return a[0];
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Let us now analyze the efficiency of this algorithm. We will begin by
analyzing the work. To start, we need to determine whether the
combining operator of the reduction over `xss` is constant-time or
not. This combining operator is not because the combining operator
calls the `max` function twice. The first call is applied to the array
`xs` and the second to `ys`. The total work performed by these two
calls is linear in $| \mathtt{xs} | + | \mathtt{ys} |$. Therefore, by
applying the work-lemma shown above, we get that the total work
performed by this reduction is $O(\log | \mathtt{xss} |
\max_{\mathtt{xs} \in \mathtt{xss}} | xs ||)$. The span is simpler to
analyze. By applying our span rule for reduce, we get that the span
for the reduction is $O(\log |xss| \max_{\mathtt{xs} \in \mathtt{xss}}
\log |xs|)$.

When the `max` function returns, the result is just one number that is
our maximum value. It is therefore unfortunate that our combining
operator has to pay to package the current maximum value in the array
`r`. The abstraction boundaries, in particular, the type of the
`reduce` function here leaves us no choice, however. In the next level
of abstraction, we are going to see that, by generalizing our `reduce`
function a little, we can sidestep this issue.

### Level 1 {#red-l-1}

***Index passing.*** TODO: explain

+----------------------------------+-----------------------------------+
| Template parameter               | Description                       |
+==================================+===================================+
| [`Result`](#r1-r)                | Type of the result value to be    |
|                                  |returned by the reduction          |
+----------------------------------+-----------------------------------+
| [`Lift`](#r1-l)                  | Lifting operator                  |
+----------------------------------+-----------------------------------+
| [`Lift_idx`](#r1-li)             | Index-passing lifting operator    |
+----------------------------------+-----------------------------------+
| [`Combine`](#r1-comb)            | Associative combining operator    |
+----------------------------------+-----------------------------------+
| [`Lift_comp`](#r1-l-c)           | Complexity function associated    |
|                                  |with the lift funciton             |
+----------------------------------+-----------------------------------+
| [`Lift_comp_idx`](#r1-l-c-i)     | Index-passing lift complexity     |
|                                  |function                           |
+----------------------------------+-----------------------------------+

Table: Template parameters that are introduced in level 1.
                 
#### Result {#r1-r}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Result               
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type of the result value to be returned by the reduction.

This class must provide a default (i.e., zero-arity) constructor.

#### Lift {#r1-l}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Lift;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The lift operator is a C++ functor that takes an iterator and returns
a value of type `Result`. The call operator for the `Lift` class
should have the following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
Result operator()(Iter it);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Index-passing lift {#r1-li}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Lift_idx;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The lift operator is a C++ functor that takes an index and a
corresponding iterator and returns a value of type `Result`. The call
operator for the `Lift` class should have the following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
Result operator()(long pos, Iter it);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The value passed in the `pos` parameter is the index corresponding to
the position of iterator `it`.

#### Associative combining operator {#r1-comb}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Combine;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Now, the type of our associative combining operator has changed from
what it is in level 0. In particular, the values that are being passed
and returned are values of type `Result`.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
Result operator()(const Result& x, const Result& y);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Complexity function for lift {#r1-l-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Lift_comp;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The lift-complexity function is a C++ functor that takes an iterator
and returns a non-negative number of type `long`. The `Lift_comp`
class should provide a call operator of the following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long operator()(Iter it);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Index-passing lift-complexity function {#r1-l-c-i}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Lift_comp_idx;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The lift-complexity function is a C++ functor that takes an index and
an iterator and returns a non-negative number of type `long`. The
`Lift_comp_idx` class should provide a call operator of the following
type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long operator()(long pos, Iter it);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Examples

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace parray {
namespace level1 {

template <
  class Iter,
  class Result,
  class Combine,
  class Lift
>
Result reduce(Iter lo,
              Iter hi,
              Result id,
              Combine combine,
              Lift lift);

template <
  class Iter,
  class Result,
  class Combine,
  class Lift_comp,
  class Lift
>
Result reduce(Iter lo,
              Iter hi,
              Result id,
              Combine combine,
              Lift_comp lift_comp,
              Lift lift);

} } } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long max(const parray<parray<long>>& xss) {
  using iterator = typename parray<parray<long>>::const_iterator;
  auto combine = [&] (long x, long y) {
    return std::max(x, y);
  };
  auto lift_comp = [&] (iterator it) {
    return it->size();
  };
  auto lift = [&] (iterator it) {
    return max(*it);
  };
  auto lo = xss.cbegin();
  auto hi = xss.cend();
  return level1::reduce(lo, hi, 0, combine, lift_comp, lift);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace parray {
namespace level1 {

template <
  class Iter,
  class Result,
  class Combine,
  class Lift_idx
>
Result reducei(Iter lo,
               Iter hi,
               Result id,
               Combine combine,
               Lift_idx lift_idx);

template <
  class Iter,
  class Result,
  class Combine,
  class Lift_comp_idx,
  class Lift_idx
>
Result reducei(Iter lo,
               Iter hi,
               Result id,
               Combine combine,
               Lift_comp_idx lift_comp_idx,
               Lift_idx lift_idx);

} } } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Level 2 {#red-l-2}

+---------------------------+-----------------------------------+
| Template parameter        | Description                       |
+===========================+===================================+
| [`Seq_lift`](#r2-l)       | Sequential alternative body for   |
|                           |the lift function                  |
+---------------------------+-----------------------------------+
| [`Lift_comp_rng`](#r2-w)  | Range-based lift complexity       |
|                           |function                           |
+---------------------------+-----------------------------------+

Table: Template parameters that are introduced in level 2.

#### Sequential alternative body for the lifting operator {#r2-l}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Seq_lift;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The sequential-lift function is a C++ functor that takes a pair of
iterators and returns a result value. The `Seq_lift` class should
provide a call operator with the following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
Result operator()(Iter lo, Iter hi);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Range-based lift-complexity function {#r2-w}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Lift_comp_rng;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The range-based lift-complexity function is a C++ functor that takes a
pair of iterators and returns a non-negative number. The value
returned is a value to account for the amount of work to be performed
to apply the lift function to the items in the right-open range `[lo,
hi)` of the input sequence.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long operator()(Iter lo, Iter hi);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Examples
             
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace parray {
namespace level2 {

template <
  class Iter,
  class Result,
  class Combine,
  class Lift_comp_rng,
  class Lift_idx,
  class Seq_lift
>
Result reduce(Iter lo,
              Iter hi,
              Result id,
              Combine combine,
              Lift_comp_rng lift_comp_rng,
              Lift_idx lift_idx,
              Seq_lift seq_lift);

} } } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following function is useful for building a table that is to
summarize the cost of processing any given range in a specified
sequence of items. The function takes as input a length value `n` and
a weight function `weight` and returns the corresponding weight table.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
template <class Weight>
parray<long> weights(long n, Weight weight);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The result returned by the call `weights(n, w)` is the sequence `[0,
w(0), w(0)+w(1), w(0)+w(1)+w(2), ..., w(0)+...+w(n-1)]`. Notice that
the size of the value returned by the `weights` function is always
`n+1`. As an example, let us consider an application of the `weights`
function where the given weight function is one that returns the value
of its current position.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
parray<long> w = weights(4, [&] (long i) {
  return i;
});
std::cout << "w = " << w << std::endl;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The input is then the following.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
w = { 0, 0, 1, 3, 6 }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long max(const parray<parray<long>>& xss) {
  using iterator = typename parray<parray<long>>::const_iterator;
  parray<long> w = weights(xss.size(), [&] (const parray<long>& xs) {
    return xs.size();
  });
  auto lift_comp_rng = [&] (iterator lo, iterator hi) {
    long l = lo - xss.cbegin();
    long h = hi - xss.cbegin();
    return w[h] - w[l];
  };
  auto combine = [&] (long x, long y) {
    return std::max(x, y);
  };
  auto lift = [&] (iterator it) {
    return max(*it);
  };
  auto seq_lift = [&] (iterator lo, iterator hi) {
    return max_seq(lo, hi);
  };
  iterator lo = xss.cbegin();
  iterator hi = xss.cend();
  return level2::reduce(lo, hi, 0, combine, lift_comp_rng, lift, seq_lift);
}

template <class Iter>
long max_seq(Iter lo, Iter hi) {
  long m = LONG_MIN;
  for (Iter it = lo; it != hi; it++) {
    const parray<long>& xs = *it;
    for (auto it2 = xs.cbegin(); it2 != xs.cend() it2++) {
      m = std::max(m, *it2);
    }
  }
  return m;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Level 3 {#red-l-3}

+----------------------------------+--------------------------------+
| Template parameter               | Description                    |
+==================================+================================+
| [`Output`](#r3-o)                | Type of the object to receive  |
|                                  |the output of the reduction     |
+----------------------------------+--------------------------------+
| [`Lift_idx_dst`](#r3-dpl)        | Lift function in               |
|                                  |destination-passing style       |
+----------------------------------+--------------------------------+
| [`Seq_lift_dst`](#r3-dpl-seq)    | Sequential lift function in    |
|                                  |destination-passing style       |
+----------------------------------+--------------------------------+

Table: Template parameters that are introduced in level 3.
                                      
#### Output {#r3-o}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Output;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type of the object to receive the output of the reduction.


+-----------------------------+-------------------------------------+
| Constructor                 | Description                         |
+==================================+================================+
| [copy constructor](#ro-c-c) | Copy constructor                    |
|                             |                                     |
+-----------------------------+-------------------------------------+

Table: Constructors that are required for the `Output` class.

Table: Required constructors for the `Output` class.

+-------------------------+-------------------------------------+
| Public method           | Description                         |
+=========================+=====================================+
| [`merge`](#ro-m)        | Merge contents                      |
+-------------------------+-------------------------------------+

Table: Public methods that are required for the `Output` class.

##### Copy constructor {#ro-c-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
Output(const Output& other);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copy constructor.

##### Merge {#ro-m}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void merge(Output& dst);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Merge the contents of the current output with those of the output
referenced by `dst`, leaving the result in `dst`.

##### Example: cell output {#ro-co}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace datapar {
namespace level3 {

template <class Result, class Combine>
class cell {
public:

  Result result;
  Combine combine;

  cell(Result result, Combine combine)
  : result(result), combine(combine) { }

  cell(const cell& other)
  : combine(other.combine) { }

  void merge(cell& dst) {
    dst.result = combine(dst.result, result);
    Result empty;
    result = empty;
  }

};

} } } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Destination-passing-style lift {#r3-dpl}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Lift_idx_dst;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The destination-passing-style lift function is a C++ functor that
takes an index, an iterator, and a reference on an output object. The
call operator for the `Lift_idx_dst` class should have the following
type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void operator()(long pos, Iter it, Output& out);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The value that is passed in for `pos` is the index in the input
sequence of the item `x`. The object referenced by `out` is the object
to receive the result of the lift function.

#### Destination-passing-style sequential lift {#r3-dpl-seq}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Seq_lift_dst;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The destination-passing-style sequential lift function is a C++
functor that takes a pair of iterators and a reference on an output
object. The call operator for the `Seq_lift_dst` class should have the
following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void operator()(Iter lo, Iter hi, Output& out);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The purpose of this function is provide an alternative sequential
algorithm that is to be used to process ranges of items from the
input. The range is specified by the right-open range `[lo, hi)`. The
object referenced by `out` is the object to receive the result of the
sequential lift function.

#### Examples

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace pctl {
namespace level3 {

template <
  class Iter,
  class Output,
  class Lift_comp_rng,
  class Lift_idx_dst,
  class Seq_lift_dst
>
void reduce(Iter lo,
            Iter hi,
            Output& out,
            Lift_comp_rng lift_comp_rng,
            Lift_idx_dst lift_idx_dst,
            Seq_lift_dst seq_lift_dst);

} } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Level 4 {#red-l-4}

+-------------------------------+-----------------------------------+
| Template parameter            | Description                       |
+===============================+===================================+
| [`Input`](#r4-i)              | Type of input to the reduction    |
+-------------------------------+-----------------------------------+
| [`Convert`](#r4-c)            | Function to convert from an input |
|                               |to an output                       |
+-------------------------------+-----------------------------------+
| [`Convert_comp`](#r4-i-w)     |  Complexity function associated   |
|                               |with the convert function          |
+-------------------------------+-----------------------------------+
| [`Seq_convert`](#r4-s-c)      | Alternative sequentialized version|
|                               |of the `Convert` function.         |
+-------------------------------+-----------------------------------+

Table: Template parameters that are introduced in level 4.

#### Input {#r4-i}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Input;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------------------+-------------------------------------------+
| Constructor                 | Description                               |
+=============================+===========================================+
| [copy constructor](#r4i-cc) | Copy constructor                          |
+-----------------------------+-------------------------------------------+

Table: Constructors that are required for the `Input` class.

+-----------------------------+-------------------------------------------+
| Public method               | Description                               |
+=============================+===========================================+
| [`can_split`](#r4i-c-s)     | Return value to indicate whether split is |
|                             |possible                                   |
+-----------------------------+-------------------------------------------+
| [`split`](#r4i-sp)          | Divide the input into two pieces          |
+-----------------------------+-------------------------------------------+

Table: Public methods that are required for the `Input` class.

##### Copy constructor {#r4i-cc}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
Input(const Input& other);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copy constructor.

##### Can split {#r4i-c-s}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
bool can_split() const;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Return a boolean value to indicate whether a split is possible.

##### Split {#r4i-sp}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void split(Input& dst);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Transfer a fraction of the contents of the current input object to the
input object referenced by `dst`.

The behavior of this method may be undefined when the `can_split`
function would return `false`.

##### Example: random-access iterator input

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace pctl {
namespace parray {
namespace level4 {

template <class Iter>
class random_access_iterator_input {
public:
  
  Iter lo;
  Iter hi;
  
  random_access_iterator_input(Iter lo, Iter hi)
  : lo(lo), hi(hi) { }
  
  bool can_split() const {
    return hi - lo >= 2;
  }
  
  void split(random_access_iterator_input& dst) {
    dst = *this;
    long n = hi - lo;
    assert(n >= 2);
    Iter mid = lo + (n / 2);
    hi = mid;
    dst.lo = mid;
  }
  
};

} } } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Convert complexity function {#r4-i-w}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Convert_comp;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The convert-complexity function is a C++ functor which returns a
positive number that associates a weight value to a given input
object. The `Convert_comp` class should provide the following call
operator.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
long operator()(const Input& in);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Convert {#r4-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Convert;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The convert function is a C++ functor which takes a reference on an
input value and computes a result value, leaving the result value in
an output cell. The `Convert` class should provide a call operator
with the following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void operator()(Input& in, Output& out);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Sequential convert {#r4-s-c}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
class Seq_convert;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The sequential convert function is a C++ functor whose purpose is to
substitute for the ordinary convert function when input size is small
enough to sequentialize. The `Seq_convert` class should provide a call
operator with the following type.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
void operator()(Input& in, Output& out);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The sequential convert function should always compute the same result
as the ordinary convert function given the same input. 

#### Examples

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ {.cpp}
namespace pasl {
namespace data {
namespace datapar {
namespace level4 {

template <
  class Input,
  class Output,
  class Convert_comp,
  class Convert,
  class Seq_convert
>
void reduce(Input& in,
            Output& out,
            Convert_comp convert_comp,
            Convert convert,
            Seq_convert seq_convert);
            
} } } }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Scan
----

Derived operations
------------------

Sorting
=======

Merge sort
----------

Quick sort
----------

Sample sort
-----------

Radix sort
----------