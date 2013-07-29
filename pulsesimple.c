/*
 * pulseaudio simple API python bindings
 * Copyright (C) 2013 Damir Jelić
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <Python.h>
#include <structmember.h>
#include <pulse/simple.h>
#include <pulse/error.h>

/*
 * pa_simple* pa_simple_new(
 *      const char *server,
 *      const char *name,
 *      pa_stream_direction_t dir,
 *      const char *dev,
 *      const char *stream_name,
 *      const pa_sample_spec *ss,
 *      const pa_channel_map *map,
 *      const pa_buffer_attr *attr,
 *      int *error
 * );
*/

typedef struct {
    PyObject_HEAD
    PyObject *server;
    PyObject *name;
    PyObject *stream_direction;
    PyObject *device;
    PyObject *description;
    PyObject *sample_spec;
    PyObject *channel_map;
    PyObject *buffer_attr;
    int error;
    pa_simple *s;
    /* Type-specific fields go here. */
} simple;

static void
PulseSimple_dealloc(simple* self)
{
    Py_XDECREF(self->server);
    Py_XDECREF(self->name);
    Py_XDECREF(self->stream_direction);
    Py_XDECREF(self->device);
    Py_XDECREF(self->description);
    Py_XDECREF(self->sample_spec);
    Py_XDECREF(self->channel_map);
    Py_XDECREF(self->buffer_attr);

    if (self->s)
        pa_simple_free(self->s);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static int simple_init(simple *self, PyObject *args, PyObject *kwds)
{
    PyObject *server=NULL, *name=NULL, *device=NULL, *direction=NULL,
             *description=NULL, *sample_spec=NULL, *channel_map=NULL,
             *buffer_attr=NULL, *tmp;

    static char *kwlist[] = {"server", "name", "direction", "device", "description",
                            "sample_spec", "channel_map", "buffer_attr", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOOOO", kwlist,
                                      &server, &name, &direction, &device,
                                      &description, &sample_spec, &channel_map,
                                      &buffer_attr))
        return -1;

    if (server) {
        tmp = self->server;
        Py_INCREF(server);
        self->server = server;
        Py_XDECREF(tmp);
    }

//    printf("name: %s\n", PyBytes_AS_STRING(name));
    if (name) {
        tmp = self->name;
        Py_INCREF(name);
        self->name = name;
        Py_XDECREF(tmp);
    }

    if (direction) {
        tmp = self->stream_direction;
        Py_INCREF(direction);
        self->stream_direction = direction;
        Py_XDECREF(tmp);
    }

    if (device) {
        tmp = self->device;
        Py_INCREF(device);
        self->device = device;
        Py_XDECREF(tmp);
    }

    if (description) {
        tmp = self->description;
        Py_INCREF(description);
        self->description = description;
        Py_XDECREF(tmp);
    }

    if (sample_spec) {
        tmp = self->sample_spec;
        Py_INCREF(sample_spec);
        self->sample_spec = sample_spec;
        Py_XDECREF(tmp);
    }

    if (channel_map) {
        tmp = self->channel_map;
        Py_INCREF(channel_map);
        self->channel_map = channel_map;
        Py_XDECREF(tmp);
    }

    if (buffer_attr) {
        tmp = self->buffer_attr;
        Py_INCREF(buffer_attr);
        self->buffer_attr = buffer_attr;
        Py_XDECREF(tmp);
    }

    return 0;
}

static PyObject *simple_connect(simple *self) {
    pa_sample_spec ss;
    char *server = NULL,
         *name = NULL,
         *device = NULL,
         *description = NULL;

    pa_stream_direction_t direction = PA_STREAM_NODIRECTION;

    if (self->s != NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Already connected to pulseaudio server");
        return Py_BuildValue("i", -1);
    }

    if (self->server)
        server = PyUnicode_AsUTF8(self->server);

    if (self->name)
        name = PyUnicode_AsUTF8(self->name);
    else {
        PyErr_SetString(PyExc_RuntimeError, "The application name can't be null");
        return Py_BuildValue("i", -2);
    }

    if (self->stream_direction) {
        char *dir = PyUnicode_AsUTF8(self->stream_direction);

        if (strcmp(dir, "playback") == 0) {
            direction = PA_STREAM_PLAYBACK;
        }
        else if (strcmp(dir, "record") == 0) {
            direction = PA_STREAM_RECORD;
        }
        else if (strcmp(dir, "upload") == 0) {
            direction = PA_STREAM_UPLOAD;
        }
        else {
            PyErr_SetString(PyExc_RuntimeError, "Invalid direction specified");
            return Py_BuildValue("i", -2);
        }
    } else {
        PyErr_SetString(PyExc_RuntimeError, "No direction specified");
        return Py_BuildValue("i", -2);
    }

    if (self->device)
        device = PyUnicode_AsUTF8(self->device);

    if (self->description)
        description = PyUnicode_AsUTF8(self->description);
    else {
        PyErr_SetString(PyExc_RuntimeError, "The stream name can't be null");
        return Py_BuildValue("i", -2);
    }

    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 1;
    ss.rate = 44100;

    self->s = pa_simple_new(server,
                            name,
                            direction,
                            device,
                            description,        // Description of our stream.
                            &ss,                // Our sample format.
                            NULL,               // Use default channel map
                            NULL,               // Use default buffering attributes.
                            &self->error
                            );

    if (self->s == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Error connecting to pulseaudio server");
        return Py_BuildValue("i", -3);
    }

    return Py_BuildValue("i", 0);
}

static PyObject *simple_write(simple *self, PyObject *args) {
    int ret = 0;
    Py_buffer buffer;

    if (!self->s) {
        PyErr_SetString(PyExc_RuntimeError, "Not connected to pulseaudio server");
        return Py_BuildValue("i", -1);
    }

    if (!PyArg_ParseTuple(args, "y*", &buffer))
        return Py_BuildValue("i", -2);

    ret = pa_simple_write(self->s, buffer.buf, buffer.len, &self->error);

    if (ret < 0) {
        PyErr_SetString(PyExc_RuntimeError, pa_strerror(self->error));
        return Py_BuildValue("i", -3);
    }

    PyBuffer_Release(&buffer);

    return Py_BuildValue("n", buffer.len);
}

static PyObject *simple_drain(simple *self) {
    int ret;

    if (!self->s) {
        PyErr_SetString(PyExc_RuntimeError, "Not connected to pulseaudio server");
        return Py_BuildValue("i", -1);
    }

    ret = pa_simple_drain(self->s, &self->error);

    if (ret < 0) {
        PyErr_SetString(PyExc_RuntimeError, pa_strerror(self->error));
        return Py_BuildValue("i", -2);
    }

    return Py_BuildValue("i", 0);
}


static PyObject *simple_disconnect(simple *self) {
    if (self->s)
        pa_simple_free(self->s);
    self->s = NULL;

    return Py_BuildValue("i", 0);
}

static PyMemberDef simple_members[] = {
    {"server", T_OBJECT_EX, offsetof(simple, server), 0,
     "server address"},
    {"name", T_OBJECT_EX, offsetof(simple, name), 0,
     "name of our application"},
    {"direction", T_OBJECT_EX, offsetof(simple, stream_direction), 0,
     "direction of the stream (play/record/upload)"},
    {"device", T_OBJECT_EX, offsetof(simple, device), 0,
     "name of the device (sink/source name) to use"},
    {NULL}
};

static PyMethodDef simple_methods[] = {
    {"connect", (PyCFunction)simple_connect, METH_NOARGS,
     "Connect to the pulseaudio server"
    },
    {"write", (PyCFunction)simple_write, METH_VARARGS,
     "Write some data to the pulseaudio server"
    },
    {"drain", (PyCFunction)simple_drain, METH_NOARGS,
     "Wait until all data writen is played by the server"
    },
    {"disconnect", (PyCFunction)simple_disconnect, METH_NOARGS,
     "Disconnect from the server"
    },
    {NULL}
};

static PyTypeObject PulseSimpleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pulsesimple.simple",                               /* tp_name */
    sizeof(simple),                                     /* tp_basicsize */
    0,                                                  /* tp_itemsize */
    (destructor)PulseSimple_dealloc,                    /* tp_dealloc */
    0,                                                  /* tp_print */
    0,                                                  /* tp_getattr */
    0,                                                  /* tp_setattr */
    0,                                                  /* tp_reserved */
    0,                                                  /* tp_repr */
    0,                                                  /* tp_as_number */
    0,                                                  /* tp_as_sequence */
    0,                                                  /* tp_as_mapping */
    0,                                                  /* tp_hash  */
    0,                                                  /* tp_call */
    0,                                                  /* tp_str */
    0,                                                  /* tp_getattro */
    0,                                                  /* tp_setattro */
    0,                                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                 /* tp_flags */
    "Pulseaudio simple connection objects",             /* tp_doc */
    0,                                                  /* tp_traverse */
    0,                                                  /* tp_clear */
    0,                                                  /* tp_richcompare */
    0,                                                  /* tp_weaklistoffset */
    0,                                                  /* tp_iter */
    0,                                                  /* tp_iternext */
    simple_methods,                                     /* tp_methods */
    simple_members,                                     /* tp_members */
    0,                                                  /* tp_getset */
    0,                                                  /* tp_base */
    0,                                                  /* tp_dict */
    0,                                                  /* tp_descr_get */
    0,                                                  /* tp_descr_set */
    0,                                                  /* tp_dictoffset */
    (initproc)simple_init,                              /* tp_init */
    0,                                                  /* tp_alloc */
    PyType_GenericNew,                                  /* tp_new */
};

static PyModuleDef pulsesimplemodule = {
    PyModuleDef_HEAD_INIT,
    "pulsesimple",
    "Python binding for the Pulseadudio simple API",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_pulsesimple(void)
{
    PyObject* m;

    PulseSimpleType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PulseSimpleType) < 0)
        return NULL;

    m = PyModule_Create(&pulsesimplemodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PulseSimpleType);
    PyModule_AddObject(m, "simple", (PyObject *)&PulseSimpleType);
    return m;
}
