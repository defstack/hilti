///
/// Code to convert between HILTI and Bro values/types.
///

#ifndef BRO_PLUGIN_HILTI_CONVERTER_H
#define BRO_PLUGIN_HILTI_CONVERTER_H

#include <hilti/hilti.h>
#include <spicy/spicy.h>

class BroType;

namespace hilti {
class Expression;
namespace builder {
class ModuleBuilder;
}
}

namespace bro {
namespace hilti {

namespace compiler {
class ModuleBuilder;
class Compiler;
}

// Converts from Spicy types to Bro types.
class TypeConverter : ast::Visitor<::hilti::AstInfo, BroType*, shared_ptr<::spicy::Type>> {
public:
    /**
     * XXX
     */
    TypeConverter(compiler::Compiler* compiler);

    /**
     * Converts a HILTI type corresponding to a Spicy type into a Bro
     * type. Aborts for unsupported types.
     *
     * @param type The HILTI type to convert.
     *
     * @param btype An optional Spicy type that \a type may correspond to.
     *
     * @return The newly allocated Bro type.
     */
    BroType* Convert(std::shared_ptr<::hilti::Type> type,
                     std::shared_ptr<::spicy::Type> btype = nullptr);

    /**
     * XXX
     */
    uint64_t TypeIndex(std::shared_ptr<::hilti::Type> type,
                       std::shared_ptr<::spicy::Type> btype = nullptr);

protected:
    void visit(::hilti::type::Address* b) override;
    void visit(::hilti::type::Bool* b) override;
    void visit(::hilti::type::Bytes* b) override;
    void visit(::hilti::type::Double* d) override;
    void visit(::hilti::type::Enum* e) override;
    void visit(::hilti::type::Integer* i) override;
    void visit(::hilti::type::String* s) override;
    void visit(::hilti::type::Time* t) override;
    void visit(::hilti::type::Interval* t) override;
    void visit(::hilti::type::Tuple* t) override;
    void visit(::hilti::type::Reference* b) override;
    void visit(::hilti::type::List* b) override;
    void visit(::hilti::type::Vector* b) override;
    void visit(::hilti::type::Set* b) override;
    void visit(::hilti::type::Struct* u) override;
    void visit(::hilti::type::Union* u) override;

private:
    string CacheIndex(std::shared_ptr<::hilti::Type> type, std::shared_ptr<::spicy::Type> btype);
    BroType* LookupCachedType(std::shared_ptr<::hilti::Type> type,
                              std::shared_ptr<::spicy::Type> btype);
    void CacheType(std::shared_ptr<::hilti::Type> type, std::shared_ptr<::spicy::Type> btype,
                   BroType* bro_type);

    compiler::Compiler* compiler = 0;

    // Caches dynamic types and their indices within the runtime's table.
    std::map<string, std::pair<BroType*, std::uint64_t>> type_cache;
};

// Converts from HILTI values to Bro values, and vice vera.
class ValueConverter : ast::Visitor<::hilti::AstInfo, bool, shared_ptr<::hilti::Expression>,
                                    shared_ptr<::hilti::Expression>> {
public:
    /**
     * XXX
     */
    ValueConverter(compiler::ModuleBuilder* mbuilder, TypeConverter* type_converter);

    /**
     * Generates HILTI code to convert a HILTI value into a Bro Val.
     * Aborts for unsupported types.
     *
     * @param value The HILTI value to convert.
     *
     * @param dst A HILTI expression referencing the location where to store the converted value.
     *
     * @param btype A Spicy type that \a value may correspond to.
     *
     * @returns True if the conversion was successul.
     */
    bool Convert(shared_ptr<::hilti::Expression> value, shared_ptr<::hilti::Expression> dst,
                 std::shared_ptr<::spicy::Type> btype, BroType* hint = nullptr);

protected:
    /**
     * Returns the Spicy type passed \a Convert() during visiting; null if none.
     */
    shared_ptr<::spicy::Type> arg3() const;

    /**
     * Returns the current block builder.
     */
    shared_ptr<::hilti::builder::BlockBuilder> Builder() const;

private:
    void visit(::hilti::type::Address* a) override;
    void visit(::hilti::type::Bool* b) override;
    void visit(::hilti::type::Bytes* b) override;
    void visit(::hilti::type::Double* d) override;
    void visit(::hilti::type::Enum* e) override;
    void visit(::hilti::type::Integer* i) override;
    void visit(::hilti::type::String* s) override;
    void visit(::hilti::type::Time* t) override;
    void visit(::hilti::type::Interval* i) override;
    void visit(::hilti::type::Tuple* t) override;
    void visit(::hilti::type::Reference* b) override;
    void visit(::hilti::type::List* b) override;
    void visit(::hilti::type::Vector* b) override;
    void visit(::hilti::type::Set* b) override;
    void visit(::hilti::type::Struct* u) override;

    compiler::ModuleBuilder* mbuilder;
    TypeConverter* type_converter;

    shared_ptr<::spicy::Type> _arg3 = nullptr;
    std::list<BroType*> _bro_type_hints;
};
}
}

#endif
