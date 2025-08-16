% Test cut operator
choice(1).
choice(2).
choice(3).

first_choice(X) :- choice(X), !.

test_cut_simple :- !, write('Cut worked'), nl.