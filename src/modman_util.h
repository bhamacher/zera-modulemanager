#ifndef MODMAN_UTIL_H
#define MODMAN_UTIL_H

#include <functional>
#include <QHash>
#include <QString>
#include <QUuid>
#include <QVariantMap>
#include <QStringLiteral>

namespace modman_util {
  ///@note used to avoid non constexpr strlen use from QLatin1String(const char[]) constructor
  template< size_t N >
  constexpr size_t string_length( char const (&)[N] )
  {
    return N-1;
  }

  template < typename T>
  constexpr QLatin1String to_latin1(T t_str) = delete;
  //for copies of other constexpr QLatin1String variables
  template<>
  constexpr QLatin1String to_latin1(const QLatin1String t_str) { return t_str; }
  //for const char[] with compile time size checking
  template<size_t N>
  constexpr QLatin1String to_latin1(const char(&t_str)[N]) { return QLatin1String(t_str, string_length(t_str)); }
}

/// helper to save time with defining component metadata
#define VF_COMPONENT(componentIdentifier, componentNameString, componentDescriptionString) \
  static constexpr QLatin1String s_##componentIdentifier##ComponentName = modman_util::to_latin1(componentNameString); \
  static constexpr QLatin1String s_##componentIdentifier##ComponentDescription = modman_util::to_latin1(componentDescriptionString);

/// helper to save time with defining rpc function metadata
#define VF_RPC(rpcIdentifier, procedureNameString, procedureDescriptionString) \
  static constexpr QLatin1String s_##rpcIdentifier##ProcedureName = modman_util::to_latin1(procedureNameString); \
  static constexpr QLatin1String s_##rpcIdentifier##ProcedureDescription = modman_util::to_latin1(procedureDescriptionString);

/// helper to set up a map or hash of the remote procedures, mainly because qt creator breaks the indentation :(
#define VF_RPC_BIND(rpcIdentifier, rpcBind) \
{ \
s_##rpcIdentifier##ProcedureName, \
rpcBind \
}

namespace VeinEvent
{
  using RoutedRemoteProcedureAtlas = QHash<QString, std::function<void(const QUuid&, QVariantMap)> >;
}

#endif // MODMAN_UTIL_H
