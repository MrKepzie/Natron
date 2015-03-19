#ifndef SBK_PATHPARAMWRAPPER_H
#define SBK_PATHPARAMWRAPPER_H

#include <shiboken.h>

#include <ParameterWrapper.h>

class PathParamWrapper : public PathParam
{
public:
    inline void _addAsDependencyOf_protected(int fromExprDimension, Param * param) { PathParam::_addAsDependencyOf(fromExprDimension, param); }
    virtual ~PathParamWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_PATHPARAMWRAPPER_H

