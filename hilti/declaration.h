
#ifndef HILTI_DECLARATION_H
#define HILTI_DECLARATION_H

#include <ast/declaration.h>

#include "common.h"
#include "expression.h"
#include "id.h"
#include "type.h"

namespace hilti {

/// Base class for AST declaration nodes.
class Declaration : public ast::Declaration<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared object.
    ///
    /// l: An associated location.
    ///
    /// \todo: We don't use the AST's new linkage attribute yet.
    Declaration(shared_ptr<hilti::ID> id, const Location& l = Location::None);

    /// Returns a readable one-line representation of the declaration.
    string render() override;

    ACCEPT_VISITOR_ROOT();

private:
};

namespace declaration {

/// AST node for declaring a variable.
class Variable : public hilti::Declaration, public ast::declaration::mixin::Variable<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared variable.
    ///
    /// var: The declared variable.
    ///
    /// l: An associated location.
    Variable(shared_ptr<hilti::ID> id, shared_ptr<hilti::Variable> var,
             const Location& l = Location::None)
        : hilti::Declaration(id, l), ast::declaration::mixin::Variable<AstInfo>(this, var)
    {
    }

    ACCEPT_VISITOR(hilti::Declaration);
};

/// AST node for declaring a constant.
class Constant : public hilti::Declaration, public ast::declaration::mixin::Constant<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared constant.
    ///
    /// expr: The declared expression.
    ///
    /// l: An associated location.
    Constant(shared_ptr<hilti::ID> id, shared_ptr<hilti::Expression> expr,
             const Location& l = Location::None)
        : hilti::Declaration(id, l), ast::declaration::mixin::Constant<AstInfo>(this, expr)
    {
    }

    ACCEPT_VISITOR(hilti::Declaration);
};

/// AST node for declaring a type.
class Type : public hilti::Declaration, public ast::declaration::mixin::Type<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared type.
    ///
    /// type: The declared type.
    ///
    /// l: An associated location.
    Type(shared_ptr<hilti::ID> id, shared_ptr<hilti::Type> type, const Location& l = Location::None)
        : hilti::Declaration(id, l), ast::declaration::mixin::Type<AstInfo>(this, type)
    {
    }

    ACCEPT_VISITOR(hilti::Declaration);
};

/// AST node for declaring a function.
class Function : public hilti::Declaration, public ast::declaration::mixin::Function<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// func: The declared function.
    ///
    /// l: An associated location.
    Function(shared_ptr<hilti::Function> func, const Location& l = Location::None);

    ACCEPT_VISITOR(hilti::Declaration);
};

/// AST node for declaring a hook. A hook implementation is just a
/// specialized function.
class Hook : public Function {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// hook: The hook.
    ///
    /// l: An associated location.
    Hook(shared_ptr<hilti::Hook> hook, const Location& l = Location::None);

    shared_ptr<hilti::Hook> hook() const;

    ACCEPT_VISITOR(Function);
};
}
}

#endif
