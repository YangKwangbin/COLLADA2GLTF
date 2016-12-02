#pragma once

#include <string>

#include "GLTFBufferView.h"
#include "GLTFConstants.h"
#include "GLTFObject.h"

namespace GLTF {
  class Accessor : public GLTF::Object {
  public:
    static int INSTANCE_COUNT;
    enum class Type {
      SCALAR,
      VEC2,
      VEC3,
      VEC4,
      MAT2,
      MAT3,
      MAT4
    };

    GLTF::BufferView* bufferView;
    int byteOffset;
    int byteStride;
    GLTF::Constants::WebGL componentType;
    int count;
	  double* max = NULL;
	  double* min = NULL;
    Type type;

    Accessor(GLTF::Accessor::Type type, GLTF::Constants::WebGL componentType);

    Accessor(GLTF::Accessor::Type type,
      GLTF::Constants::WebGL componentType,
      unsigned char* data,
      int count,
      GLTF::Constants::WebGL target
    );

    Accessor(GLTF::Accessor::Type type,
      GLTF::Constants::WebGL componentType,
      unsigned char* data,
      int count,
      GLTF::BufferView* bufferView
    );

    bool computeMinMax();
    int getByteStride();
    bool getComponentAtIndex(int index, double* component);
	static int getComponentByteLength(GLTF::Constants::WebGL componentType);
    int getComponentByteLength();
	static int getNumberOfComponents(GLTF::Accessor::Type type);
    int getNumberOfComponents();
  };
};
