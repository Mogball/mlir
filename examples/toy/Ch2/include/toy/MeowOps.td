#ifdef MEOW_OPS
#else
#define MEOW_OPS

#ifdef OP_BASE
#else
include "mlir/IR/OpBase.td"
#endif // OP_BASE

def Meow_Dialect : Dialect {
  let name = "meow";
  let cppNamespace = "meow";
}

class Meow_Op<string mnemonic, list<OpTrait> traits = []> :
    Op<Meow_Dialect, mnemonic, traits>;

def MeowOp : Meow_Op<"meow", [NoSideEffect]> {
  let summary = "meow";
  let description = [{
    Do something meow!
  }];

  let arguments = (ins StrAttr:$value);
  let results = (outs);

}

def RetOp : Meow_Op<"ret", [Terminator, HasParent<"FuncOp">]> {
  let summary = "return operation";
  let description = [{ Toplevel return op }];

  let arguments = (ins);

}

#endif // MEOW_OPS
