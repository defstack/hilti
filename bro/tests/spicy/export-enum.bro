#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./tupleenum.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

event bro_init() {
  local i: TupleEnum::TestEnum;

  i = TupleEnum::TestEnum_A;

  print i;
}

# @TEST-START-FILE tupleenum.evt

grammar tupleenum.spicy;

# @TEST-END-FILE

# @TEST-START-FILE tupleenum.spicy

module TupleEnum;

export type TestEnum = enum {
    A = 83, B = 84, C = 85
};

# @TEST-END-FILE
