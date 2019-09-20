DagIV iv0 = {false, 0, "dageq.test0", {&input0, &input1}};
DagIV iv1 = {false, 1, "dageq.test1", {&input2, &input3}};
DagIV out = {false, 2, "dageq.test2", {&iv0, &iv1}};
DagIV rep = {false, 3, "dageq.iWasReplaced", {&input0, &input1, &input2, &input3}};

func @function0(%arg0 : i32, %arg1 : i32, %arg2 : i32, %arg3 : i32) -> i32 {
  // CHECK: %2 = "dageq.iWasReplaced"(%arg0, %arg1, %arg2, %arg3)
  %0 = "dageq.test0"(%arg0, %arg1) : (i32, i32) -> i32
  %1 = "dageq.test0"(%arg2, %arg3) : (i32, i32) -> i32
  %2 = "dageq.test1"(%0, %1) : (i32, i32) -> i32

  // CHECK: %4 = "dageq.iWasReplaced"(%arg2, %arg0, %arg2, %arg0)
  %3 = "dageq.test0"(%arg2, %arg0) : (i32, i32) -> i32
  %4 = "dageq.test1"(%0, %0) : (i32, i32) -> i32

  // CHECK: %7 = "dageq.iWasReplaced"(%arg1, %arg2, %arg0, %arg2)
  // CHECK: %8 = "dageq.iWasReplaced"(%arg0, %arg2, %arg1, %arg2)
  %5 = "dageq.test0"(%arg0, %arg2) : (i32, i32) -> i32
  %6 = "dageq.test0"(%arg1, %arg2) : (i32, i32) -> i32
  %7 = "dageq.test1"(%6, %5) : (i32, i32) -> i32
  %8 = "dageq.test1"(%5, %6) : (i32, i32) -> i32

  // CHECK: %9 = "dageq.test1"(%5, %6, %arg0)
  %9 = "dageq.test1"(%5, %6, %arg0) : (i32, i32, i32) -> i32

  // CHECK: %10 = "dageq.test1"(%0, %arg0)
  %10 = "dageq.test1"(%0, %arg0) : (i32, i32) -> i32

  return %8 : i32
}

func @function1(%arg0 : i32, %arg1 : i32, %arg2 : i32, %arg3 : i32) -> i32 {
  // CHECK: %3 = "dageq.iWasReplaced"(%0, %arg2, %arg2, %arg3)
  // CHECK: %4 = "dageq.iWasReplaced"(%arg0, %arg1, %0, %arg2)
  %0 = "dageq.test0"(%arg0, %arg1) : (i32, i32) -> i32
  %1 = "dageq.test0"(%arg2, %arg3) : (i32, i32) -> i32
  %2 = "dageq.test0"(%0, %arg2) : (i32, i32) -> i32
  %3 = "dageq.test1"(%2, %1) : (i32, i32) -> i32
  %4 = "dageq.test1"(%0, %2) : (i32, i32) -> i32

  // CHECK: %5 = "dageq.test1"(%3, %4)
  %5 = "dageq.test1"(%3, %4) : (i32, i32) -> i32
}
