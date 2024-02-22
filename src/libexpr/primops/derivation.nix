/* This is the implementation of the ‘derivation’ builtin function.
   It's actually a wrapper around the ‘derivationStrict’ primop. */
let
  inherit (builtins) listToAttrs head;
  defaultOutputs = [ "out" ];
in
drvAttrs @ { outputs ? defaultOutputs, ... }:
let
  strict = derivationStrict drvAttrs;

  commonAttrs = drvAttrs // (listToAttrs outputsList) //
    { all = map (x: x.value) outputsList;
      inherit drvAttrs;
      drvPath = strict.drvPath;
      type = "derivation";
    };

  outputToAttrListElement = outputName:
    { name = outputName;
      value = commonAttrs // {
        outPath = strict.${outputName};
        inherit outputName;
      };
    };

  outputsList = map outputToAttrListElement outputs;

in (head outputsList).value
