
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "recti_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <RectI.h>


// Native ---------------------------------------------------------

void RectIWrapper::pysideInitQtMetaTypes()
{
}

RectIWrapper::RectIWrapper() : RectI() {
    // ... middle
}

RectIWrapper::RectIWrapper(int l, int b, int r, int t) : RectI(l, b, r, t) {
    // ... middle
}

RectIWrapper::~RectIWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_RectI_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::RectI >()))
        return -1;

    ::RectIWrapper* cptr = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectI_Init_TypeError;

    if (!PyArg_UnpackTuple(args, "RectI", 0, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return -1;


    // Overloaded function decisor
    // 0: RectI()
    // 1: RectI(RectI)
    // 2: RectI(int,int,int,int)
    if (numArgs == 0) {
        overloadId = 0; // RectI()
    } else if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 2; // RectI(int,int,int,int)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArgs[0])))) {
        overloadId = 1; // RectI(RectI)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectI_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // RectI()
        {

            if (!PyErr_Occurred()) {
                // RectI()
                cptr = new ::RectIWrapper();
            }
            break;
        }
        case 1: // RectI(const RectI & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::RectI* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // RectI(RectI)
                cptr = new ::RectIWrapper(*cppArg0);
            }
            break;
        }
        case 2: // RectI(int l, int b, int r, int t)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // RectI(int,int,int,int)
                cptr = new ::RectIWrapper(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::RectI >(), cptr)) {
        delete cptr;
        return -1;
    }
    if (!cptr) goto Sbk_RectI_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_RectI_Init_TypeError:
        const char* overloads[] = {"", "NatronEngine.RectI", "int, int, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectI", overloads);
        return -1;
}

static PyObject* Sbk_RectIFunc_bottom(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // bottom()const
            int cppResult = const_cast<const ::RectI*>(cppSelf)->bottom();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectIFunc_clear(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // clear()
            cppSelf->clear();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_RectIFunc_contains(PyObject* self, PyObject* args)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "contains", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: contains(RectI)const
    // 1: contains(double,double)const
    // 2: contains(int,int)const
    if (numArgs == 2
        && PyFloat_Check(pyArgs[0]) && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 1; // contains(double,double)const
    } else if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 2; // contains(int,int)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArgs[0])))) {
        overloadId = 0; // contains(RectI)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_contains_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // contains(const RectI & other) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectI* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // contains(RectI)const
                bool cppResult = const_cast<const ::RectI*>(cppSelf)->contains(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // contains(double x, double y) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // contains(double,double)const
                bool cppResult = const_cast<const ::RectI*>(cppSelf)->contains(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 2: // contains(int x, int y) const
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // contains(int,int)const
                bool cppResult = const_cast<const ::RectI*>(cppSelf)->contains(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RectIFunc_contains_TypeError:
        const char* overloads[] = {"NatronEngine.RectI", "float, float", "int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectI.contains", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_height(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // height()const
            int cppResult = const_cast<const ::RectI*>(cppSelf)->height();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectIFunc_intersect(PyObject* self, PyObject* args)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_intersect_TypeError;

    if (!PyArg_UnpackTuple(args, "intersect", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: intersect(RectI,RectI*)const
    // 1: intersect(int,int,int,int,RectI*)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // intersect(int,int,int,int,RectI*)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArgs[0])))) {
        overloadId = 0; // intersect(RectI,RectI*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_intersect_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersect(const RectI & r, RectI * intersection) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectI* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersect(RectI,RectI*)const
                // Begin code injection

                RectI t;
                cppSelf->intersect(*cppArg0,&t);
                pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], &t);
                return pyResult;

                // End of code injection


            }
            break;
        }
        case 1: // intersect(int l, int b, int r, int t, RectI * intersection) const
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // intersect(int,int,int,int,RectI*)const
                // Begin code injection

                RectI t;
                cppSelf->intersect(cppArg0,cppArg1,cppArg2,cppArg3,&t);
                pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], &t);
                return pyResult;

                // End of code injection


            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RectIFunc_intersect_TypeError:
        const char* overloads[] = {"NatronEngine.RectI, NatronEngine.RectI", "int, int, int, int, NatronEngine.RectI", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectI.intersect", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_intersects(PyObject* self, PyObject* args)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_intersects_TypeError;

    if (!PyArg_UnpackTuple(args, "intersects", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: intersects(RectI)const
    // 1: intersects(int,int,int,int)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // intersects(int,int,int,int)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArgs[0])))) {
        overloadId = 0; // intersects(RectI)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_intersects_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersects(const RectI & r) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectI* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersects(RectI)const
                bool cppResult = const_cast<const ::RectI*>(cppSelf)->intersects(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // intersects(int l, int b, int r, int t) const
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // intersects(int,int,int,int)const
                bool cppResult = const_cast<const ::RectI*>(cppSelf)->intersects(cppArg0, cppArg1, cppArg2, cppArg3);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RectIFunc_intersects_TypeError:
        const char* overloads[] = {"NatronEngine.RectI", "int, int, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectI.intersects", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_isInfinite(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isInfinite()const
            bool cppResult = const_cast<const ::RectI*>(cppSelf)->isInfinite();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectIFunc_isNull(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isNull()const
            bool cppResult = const_cast<const ::RectI*>(cppSelf)->isNull();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectIFunc_left(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // left()const
            int cppResult = const_cast<const ::RectI*>(cppSelf)->left();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectIFunc_merge(PyObject* self, PyObject* args)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_merge_TypeError;

    if (!PyArg_UnpackTuple(args, "merge", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: merge(RectI)
    // 1: merge(int,int,int,int)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // merge(int,int,int,int)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArgs[0])))) {
        overloadId = 0; // merge(RectI)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_merge_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // merge(const RectI & box)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectI* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // merge(RectI)
                cppSelf->merge(*cppArg0);
            }
            break;
        }
        case 1: // merge(int l, int b, int r, int t)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // merge(int,int,int,int)
                cppSelf->merge(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_merge_TypeError:
        const char* overloads[] = {"NatronEngine.RectI", "int, int, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectI.merge", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_right(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // right()const
            int cppResult = const_cast<const ::RectI*>(cppSelf)->right();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectIFunc_set(PyObject* self, PyObject* args)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_set_TypeError;

    if (!PyArg_UnpackTuple(args, "set", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: set(RectI)
    // 1: set(int,int,int,int)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // set(int,int,int,int)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArgs[0])))) {
        overloadId = 0; // set(RectI)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const RectI & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::RectI* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(RectI)
                cppSelf->set(*cppArg0);
            }
            break;
        }
        case 1: // set(int l, int b, int r, int t)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // set(int,int,int,int)
                cppSelf->set(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_TypeError:
        const char* overloads[] = {"NatronEngine.RectI", "int, int, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectI.set", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_set_bottom(PyObject* self, PyObject* pyArg)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_bottom(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_bottom(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_bottom_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_bottom(int)
            cppSelf->set_bottom(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_bottom_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectI.set_bottom", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_set_left(PyObject* self, PyObject* pyArg)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_left(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_left(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_left_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_left(int)
            cppSelf->set_left(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_left_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectI.set_left", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_set_right(PyObject* self, PyObject* pyArg)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_right(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_right(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_right_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_right(int)
            cppSelf->set_right(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_right_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectI.set_right", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_set_top(PyObject* self, PyObject* pyArg)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: set_top(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_top(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_top_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_top(int)
            cppSelf->set_top(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_top_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.RectI.set_top", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_top(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // top()const
            int cppResult = const_cast<const ::RectI*>(cppSelf)->top();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_RectIFunc_translate(PyObject* self, PyObject* args)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "translate", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: translate(int,int)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // translate(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_translate_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // translate(int,int)
            cppSelf->translate(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_translate_TypeError:
        const char* overloads[] = {"int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RectI.translate", overloads);
        return 0;
}

static PyObject* Sbk_RectIFunc_width(PyObject* self)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // width()const
            int cppResult = const_cast<const ::RectI*>(cppSelf)->width();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_RectI_methods[] = {
    {"bottom", (PyCFunction)Sbk_RectIFunc_bottom, METH_NOARGS},
    {"clear", (PyCFunction)Sbk_RectIFunc_clear, METH_NOARGS},
    {"contains", (PyCFunction)Sbk_RectIFunc_contains, METH_VARARGS},
    {"height", (PyCFunction)Sbk_RectIFunc_height, METH_NOARGS},
    {"intersect", (PyCFunction)Sbk_RectIFunc_intersect, METH_VARARGS},
    {"intersects", (PyCFunction)Sbk_RectIFunc_intersects, METH_VARARGS},
    {"isInfinite", (PyCFunction)Sbk_RectIFunc_isInfinite, METH_NOARGS},
    {"isNull", (PyCFunction)Sbk_RectIFunc_isNull, METH_NOARGS},
    {"left", (PyCFunction)Sbk_RectIFunc_left, METH_NOARGS},
    {"merge", (PyCFunction)Sbk_RectIFunc_merge, METH_VARARGS},
    {"right", (PyCFunction)Sbk_RectIFunc_right, METH_NOARGS},
    {"set", (PyCFunction)Sbk_RectIFunc_set, METH_VARARGS},
    {"set_bottom", (PyCFunction)Sbk_RectIFunc_set_bottom, METH_O},
    {"set_left", (PyCFunction)Sbk_RectIFunc_set_left, METH_O},
    {"set_right", (PyCFunction)Sbk_RectIFunc_set_right, METH_O},
    {"set_top", (PyCFunction)Sbk_RectIFunc_set_top, METH_O},
    {"top", (PyCFunction)Sbk_RectIFunc_top, METH_NOARGS},
    {"translate", (PyCFunction)Sbk_RectIFunc_translate, METH_VARARGS},
    {"width", (PyCFunction)Sbk_RectIFunc_width, METH_NOARGS},

    {0} // Sentinel
};

// Rich comparison
static PyObject* Sbk_RectI_richcompare(PyObject* self, PyObject* pyArg, int op)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    ::RectI& cppSelf = *(((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self)));
    SBK_UNUSED(cppSelf)
    PyObject* pyResult = 0;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    switch (op) {
        case Py_NE:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArg)))) {
                // operator!=(const RectI & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return 0;
                ::RectI* cppArg0;
                pythonToCpp(pyArg, &cppArg0);
                bool cppResult = cppSelf != (*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_True;
                Py_INCREF(pyResult);
            }

            break;
        case Py_EQ:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], (pyArg)))) {
                // operator==(const RectI & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return 0;
                ::RectI* cppArg0;
                pythonToCpp(pyArg, &cppArg0);
                bool cppResult = cppSelf == (*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_False;
                Py_INCREF(pyResult);
            }

            break;
        default:
            goto Sbk_RectI_RichComparison_TypeError;
    }

    if (pyResult && !PyErr_Occurred())
        return pyResult;
    Sbk_RectI_RichComparison_TypeError:
    PyErr_SetString(PyExc_NotImplementedError, "operator not implemented.");
    return 0;

}

static PyObject* Sbk_RectI_get_y1(PyObject* self, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->y1;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_y1(PyObject* self, PyObject* pyIn, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'y1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y1', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->y1;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->y1 = cppOut_local;

    return 0;
}

static PyObject* Sbk_RectI_get_x1(PyObject* self, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->x1;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_x1(PyObject* self, PyObject* pyIn, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'x1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x1', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->x1;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->x1 = cppOut_local;

    return 0;
}

static PyObject* Sbk_RectI_get_y2(PyObject* self, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->y2;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_y2(PyObject* self, PyObject* pyIn, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'y2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y2', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->y2;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->y2 = cppOut_local;

    return 0;
}

static PyObject* Sbk_RectI_get_x2(PyObject* self, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->x2;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_x2(PyObject* self, PyObject* pyIn, void*)
{
    ::RectI* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RectI*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'x2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x2', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->x2;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->x2 = cppOut_local;

    return 0;
}

// Getters and Setters for RectI
static PyGetSetDef Sbk_RectI_getsetlist[] = {
    {const_cast<char*>("y1"), Sbk_RectI_get_y1, Sbk_RectI_set_y1},
    {const_cast<char*>("x1"), Sbk_RectI_get_x1, Sbk_RectI_set_x1},
    {const_cast<char*>("y2"), Sbk_RectI_get_y2, Sbk_RectI_set_y2},
    {const_cast<char*>("x2"), Sbk_RectI_get_x2, Sbk_RectI_set_x2},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_RectI_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_RectI_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_RectI_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.RectI",
    /*tp_basicsize*/        sizeof(SbkObject),
    /*tp_itemsize*/         0,
    /*tp_dealloc*/          &SbkDeallocWrapper,
    /*tp_print*/            0,
    /*tp_getattr*/          0,
    /*tp_setattr*/          0,
    /*tp_compare*/          0,
    /*tp_repr*/             0,
    /*tp_as_number*/        0,
    /*tp_as_sequence*/      0,
    /*tp_as_mapping*/       0,
    /*tp_hash*/             0,
    /*tp_call*/             0,
    /*tp_str*/              0,
    /*tp_getattro*/         0,
    /*tp_setattro*/         0,
    /*tp_as_buffer*/        0,
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_RectI_traverse,
    /*tp_clear*/            Sbk_RectI_clear,
    /*tp_richcompare*/      Sbk_RectI_richcompare,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_RectI_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_RectI_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_RectI_Init,
    /*tp_alloc*/            0,
    /*tp_new*/              SbkObjectTpNew,
    /*tp_free*/             0,
    /*tp_is_gc*/            0,
    /*tp_bases*/            0,
    /*tp_mro*/              0,
    /*tp_cache*/            0,
    /*tp_subclasses*/       0,
    /*tp_weaklist*/         0
}, },
    /*priv_data*/           0
};
} //extern


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void RectI_PythonToCpp_RectI_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_RectI_Type, pyIn, cppOut);
}
static PythonToCppFunc is_RectI_PythonToCpp_RectI_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_RectI_Type))
        return RectI_PythonToCpp_RectI_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* RectI_PTR_CppToPython_RectI(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::RectI*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_RectI_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_RectI(PyObject* module)
{
    SbkNatronEngineTypes[SBK_RECTI_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_RectI_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "RectI", "RectI*",
        &Sbk_RectI_Type, &Shiboken::callCppDestructor< ::RectI >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_RectI_Type,
        RectI_PythonToCpp_RectI_PTR,
        is_RectI_PythonToCpp_RectI_PTR_Convertible,
        RectI_PTR_CppToPython_RectI);

    Shiboken::Conversions::registerConverterName(converter, "RectI");
    Shiboken::Conversions::registerConverterName(converter, "RectI*");
    Shiboken::Conversions::registerConverterName(converter, "RectI&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectI).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectIWrapper).name());



    RectIWrapper::pysideInitQtMetaTypes();
}
