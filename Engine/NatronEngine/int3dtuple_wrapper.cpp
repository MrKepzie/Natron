
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "int3dtuple_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_Int3DTuple_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::Int3DTuple >()))
        return -1;

    ::Int3DTuple* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // Int3DTuple()
            cptr = new ::Int3DTuple();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::Int3DTuple >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyMethodDef Sbk_Int3DTuple_methods[] = {

    {0} // Sentinel
};

PyObject* Sbk_Int3DTupleFunc___getitem__(PyObject* self, Py_ssize_t _i)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    ::Int3DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int3DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], (SbkObject*)self));
    // Begin code injection

    if (_i < 0 || _i >= 3) {
    PyErr_BadArgument();
    return 0;
    } else {
    int ret;
    switch (_i) {
    case 0:
    ret = cppSelf->x;
    break;
    case 1:
    ret = cppSelf->y;
    break;
    case 2:
    ret = cppSelf->z;
    break;
    }
    return  Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &ret);
    }

    // End of code injection
}

static PyObject* Sbk_Int3DTuple_get_x(PyObject* self, void*)
{
    ::Int3DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int3DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->x;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_Int3DTuple_set_x(PyObject* self, PyObject* pyIn, void*)
{
    ::Int3DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int3DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'x' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->x;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->x = cppOut_local;

    return 0;
}

static PyObject* Sbk_Int3DTuple_get_y(PyObject* self, void*)
{
    ::Int3DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int3DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->y;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_Int3DTuple_set_y(PyObject* self, PyObject* pyIn, void*)
{
    ::Int3DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int3DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'y' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->y;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->y = cppOut_local;

    return 0;
}

static PyObject* Sbk_Int3DTuple_get_z(PyObject* self, void*)
{
    ::Int3DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int3DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->z;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_Int3DTuple_set_z(PyObject* self, PyObject* pyIn, void*)
{
    ::Int3DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int3DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'z' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'z', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->z;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->z = cppOut_local;

    return 0;
}

// Getters and Setters for Int3DTuple
static PyGetSetDef Sbk_Int3DTuple_getsetlist[] = {
    {const_cast<char*>("x"), Sbk_Int3DTuple_get_x, Sbk_Int3DTuple_set_x},
    {const_cast<char*>("y"), Sbk_Int3DTuple_get_y, Sbk_Int3DTuple_set_y},
    {const_cast<char*>("z"), Sbk_Int3DTuple_get_z, Sbk_Int3DTuple_set_z},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_Int3DTuple_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Int3DTuple_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static PySequenceMethods Sbk_Int3DTuple_TypeAsSequence;

static SbkObjectType Sbk_Int3DTuple_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Int3DTuple",
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
    /*tp_traverse*/         Sbk_Int3DTuple_traverse,
    /*tp_clear*/            Sbk_Int3DTuple_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Int3DTuple_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_Int3DTuple_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_Int3DTuple_Init,
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
static void Int3DTuple_PythonToCpp_Int3DTuple_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Int3DTuple_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Int3DTuple_PythonToCpp_Int3DTuple_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Int3DTuple_Type))
        return Int3DTuple_PythonToCpp_Int3DTuple_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Int3DTuple_PTR_CppToPython_Int3DTuple(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Int3DTuple*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Int3DTuple_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Int3DTuple(PyObject* module)
{
    // type supports sequence protocol
    memset(&Sbk_Int3DTuple_TypeAsSequence, 0, sizeof(PySequenceMethods));
    Sbk_Int3DTuple_TypeAsSequence.sq_item = &Sbk_Int3DTupleFunc___getitem__;
    Sbk_Int3DTuple_Type.super.ht_type.tp_as_sequence = &Sbk_Int3DTuple_TypeAsSequence;

    SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Int3DTuple_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Int3DTuple", "Int3DTuple*",
        &Sbk_Int3DTuple_Type, &Shiboken::callCppDestructor< ::Int3DTuple >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Int3DTuple_Type,
        Int3DTuple_PythonToCpp_Int3DTuple_PTR,
        is_Int3DTuple_PythonToCpp_Int3DTuple_PTR_Convertible,
        Int3DTuple_PTR_CppToPython_Int3DTuple);

    Shiboken::Conversions::registerConverterName(converter, "Int3DTuple");
    Shiboken::Conversions::registerConverterName(converter, "Int3DTuple*");
    Shiboken::Conversions::registerConverterName(converter, "Int3DTuple&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Int3DTuple).name());



}
