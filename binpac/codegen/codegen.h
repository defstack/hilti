
#ifndef BINPAC_CODEGEN_CODEGEN_H
#define BINPAC_CODEGEN_CODEGEN_H

#include "../common.h"

namespace hilti {
    class Expression;
    class Module;
    class Type;

    namespace builder {
        class ModuleBuilder;
        class BlockBuilder;
    }
}

namespace binpac {

namespace codegen {
    class CoercionBuilder;
    class CodeBuilder;
    class TypeBuilder;
}

class CodeGen : public ast::Logger
{
public:
    /// Constructor.
    CodeGen();
    virtual ~CodeGen();

    /// Compiles a BinPAC++ module into a HILTI module.
    ///
    /// module: The module to compile.
    ///
    /// debug: Debug level for the generated code.
    ///
    /// verify: True if the generated HILTI code should be run HILTI's verifier (usually a good idea...)
    ///
    /// Returns: The HILTI module, or null if an error occured.
    shared_ptr<hilti::Module> compile(shared_ptr<Module> module, int debug, bool verify);

    /// Returns the current HILTI module builder.
    shared_ptr<hilti::builder::ModuleBuilder> moduleBuilder() const;

    /// Returns the current HILTI block builder.
    shared_ptr<hilti::builder::BlockBuilder> builder() const;

    /// Returns the HILTI epxression resulting from evaluating a BinPAC
    /// expression. The method adds the code to the current HILTI block.
    ///
    /// expr: The expression to evaluate.
    ///
    /// coerce_to: If given, the expr is first coerced into this type before
    /// it's evaluated. It's assumed that the coercion is legal and supported.
    ///
    /// Returns: The computed HILTI expression.
    shared_ptr<hilti::Expression> hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to = nullptr);

    /// Compiles a BinPAC statement into HILTI. The method adds the code to
    /// current HILTI block.
    ///
    /// stmt: The statement to compile
    void hiltiStatement(shared_ptr<Statement> stmt);

    /// Converts a BinPAC type into its corresponding HILTI type.
    ///
    /// type: The type to convert.
    ///
    /// Returns: The HILTI type.
    shared_ptr<hilti::Type> hiltiType(shared_ptr<Type> type);

    /// Coerces a HILTI expression of one BinPAC type into another. The
    /// method assumes that the coercion is legel.
    ///
    /// expr: The HILTI expression to coerce.
    ///
    /// src: The BinPAC type that \a expr currently has.
    ///
    /// dst: The BinPAC type that \a expr is to be coerced into.
    ///
    /// Returns: The coerced expression.
    shared_ptr<hilti::Expression> hiltiCoerce(shared_ptr<hilti::Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst);

private:
    bool _compiling = false;

    int _debug;
    bool _verify;

    shared_ptr<hilti::builder::ModuleBuilder> _mbuilder;

    unique_ptr<codegen::CoercionBuilder>     _coercion_builder;
    unique_ptr<codegen::CodeBuilder> _code_builder;
    unique_ptr<codegen::TypeBuilder> _type_builder;
};

}

#endif
