#include <Python.h>

int great_function_from_python(int a)
{
	int res;
	PyObject *pModule,*pFunc;
	PyObject *pArgs,*pValue;

	pModule = PyImport_Import(PyString_FromString("great_module"));

	pFunc = PyObject_GetAttrString(pModule,"great_function");

	pArgs = PyTuple_New(1);
	PyTuple_SetItem(pArgs,0,PyInt_FromLong(a));

	pValue = PyObject_CallObject(pFunc,pArgs);

	res = PyInt_AsLong(pValue);
	return res;
}

int main(int argc,char *argv[])
{
	printf("fuck1");
	Py_Initialize();
	if(Py_IsInitialized()){
		printf("fuck2");
	}
	printf("%d\n",great_function_from_python(2));
	//great_function_from_python();
	Py_Finalize();
}
