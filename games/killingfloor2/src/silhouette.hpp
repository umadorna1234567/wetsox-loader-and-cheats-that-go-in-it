#pragma once
#include "silhouette_math.hpp"
#include <map>
namespace kf2 {
struct SkinVertex {Vec point{};std::array<unsigned short,4> bones{};std::array<unsigned char,4> weights{};};
struct SilhouetteGeometry {
 Ptr asset{},type{};Name identity{};std::vector<SkinVertex> vertices;std::vector<unsigned short> indices;std::vector<BoneAtom> inverse;
};
template<class T>bool readGeometryArray(Ptr data,int count,int maximum,std::vector<T>& out){
 if(!data||count<=0||count>maximum)return false;out.resize(count);return copyReadable(data,out.data(),out.size()*sizeof(T));
}
inline bool loadSilhouetteGeometry(Ptr asset,int lodIndex,SilhouetteGeometry& out){
 out={};out.asset=asset;out.type=objectClass(asset);out.identity=read<Name>(asset+0x48);
 const auto lods=get<Array>(asset,"LODModels"),inverse=get<Array>(asset,"RefBasesInvMatrix");
 if(!lods.data||lods.count<1||lods.count>8||!readGeometryArray(inverse.data,inverse.count,512,out.inverse))return false;
 // Verified KF2 x64 native LOD layout; every buffer and range is validated.
 // Match the renderer's active LOD: higher-detail geometry may reference
 // head/toe bones omitted from RequiredBones and left at an old pose.
 if(lodIndex<0||lodIndex>=lods.count)return false;
 auto lod=read<Ptr>(lods.data+sizeof(Ptr)*lodIndex);if(!lod)return false;
 const auto chunks=read<Array>(lod+0x10);const auto sections=read<Array>(lod);
 const int vertexCount=read<int>(lod+0x54),stride=read<int>(lod+0xbc),gpuCount=read<int>(lod+0xc0);
 if(vertexCount<3||vertexCount>40000||gpuCount!=vertexCount||(stride!=32&&stride!=36&&stride!=40&&stride!=44)||chunks.count<1||chunks.count>128||sections.count<1||sections.count>128)return false;
 // Only the verified 16-bit index buffer format is accepted.
 if(read<unsigned>(lod+0x40)!=1)return false;
 const auto indexObject=read<Ptr>(lod+0x48);const auto indexArray=read<Array>(indexObject+0x40);
 if(!readGeometryArray(indexArray.data,indexArray.count,240000,out.indices)||out.indices.size()%3)return false;
 for(auto index:out.indices)if(index>=vertexCount)return false;
 std::vector<unsigned char> raw;if(!readGeometryArray(read<Ptr>(lod+0xb4),vertexCount*stride,1760000,raw))return false;
 std::vector<unsigned char> chunkBytes;if(!readGeometryArray(chunks.data,chunks.count*64,8192,chunkBytes))return false;
 std::vector<unsigned char> sectionBytes;if(!readGeometryArray(sections.data,sections.count*16,2048,sectionBytes))return false;
 std::vector<unsigned char> assigned(out.indices.size()/3,0);
 for(int i=0;i<sections.count;++i){auto at=sectionBytes.data()+16*i;unsigned short chunk{};unsigned start{},triangles{};
  std::memcpy(&chunk,at+2,2);std::memcpy(&start,at+4,4);std::memcpy(&triangles,at+8,4);
  if(chunk>=chunks.count||start%3||start>out.indices.size()||triangles>(out.indices.size()-start)/3)return false;
  int base{},rigid{},soft{};auto source=chunkBytes.data()+64*chunk;
  std::memcpy(&base,source,4);std::memcpy(&rigid,source+0x34,4);std::memcpy(&soft,source+0x38,4);
  if(base<0||rigid<0||soft<0||rigid>vertexCount||soft>vertexCount||base>vertexCount-rigid-soft)return false;
  for(unsigned t=start/3;t<start/3+triangles;++t){if(assigned[t]++)return false;
   for(int j=0;j<3;++j){auto& index=out.indices[t*3+j];if(index>=rigid+soft)return false;index=static_cast<unsigned short>(index+base);}
  }
 }
 if(std::find(assigned.begin(),assigned.end(),0)!=assigned.end())return false;
 out.vertices.resize(vertexCount);int covered=0;
 for(int c=0;c<chunks.count;++c){
  auto at=chunkBytes.data()+64*c;int base{},rigid{},soft{};Array boneMap{};
  std::memcpy(&base,at,4);std::memcpy(&boneMap,at+0x24,16);std::memcpy(&rigid,at+0x34,4);std::memcpy(&soft,at+0x38,4);
  if(base!=covered||rigid<0||soft<0||rigid>vertexCount||soft>vertexCount||rigid+soft>vertexCount-base)return false;
  std::vector<unsigned short> bones;if(!readGeometryArray(boneMap.data,boneMap.count,256,bones))return false;
  for(auto b:bones)if(b>=out.inverse.size())return false;
  for(int i=base;i<base+rigid+soft;++i){auto source=raw.data()+i*stride;auto& v=out.vertices[i];std::memcpy(&v.point,source+16,12);
   if(!finite(v.point)||length(v.point)>100000)return false;int weight=0;
   for(int j=0;j<4;++j){v.weights[j]=source[12+j];weight+=v.weights[j];if(!v.weights[j])continue;if(source[8+j]>=bones.size())return false;v.bones[j]=bones[source[8+j]];}
   if(weight<250||weight>260)return false;
  }
  covered=base+rigid+soft;
 }
 return covered==vertexCount;
}
inline bool silhouettePositions(Ptr mesh,const SilhouetteGeometry& geometry,std::vector<Vec>& out){
 auto bases=get<Array>(mesh,"SpaceBases");std::vector<BoneAtom> pose;
 if(bases.count!=int(geometry.inverse.size())||!readGeometryArray(bases.data,bases.count,512,pose))return false;
 const auto world=get<Matrix>(mesh,"LocalToWorld");for(auto f:world.m)if(!std::isfinite(f))return false;
 struct Affine {Vec origin,x,y,z;};std::vector<Affine> transforms(pose.size());
 auto vectorToWorld=[&](Vec p){return Vec{p.x*world.m[0]+p.y*world.m[4]+p.z*world.m[8],p.x*world.m[1]+p.y*world.m[5]+p.z*world.m[9],p.x*world.m[2]+p.y*world.m[6]+p.z*world.m[10]};};
 for(size_t b=0;b<pose.size();++b){auto transform=[&](Vec p){return transformBone(pose[b],transformBone(geometry.inverse[b],p));};
  auto& a=transforms[b];auto origin=transform({});a.origin=transformPoint(world,origin);a.x=vectorToWorld(transform({1,0,0})-origin);a.y=vectorToWorld(transform({0,1,0})-origin);a.z=vectorToWorld(transform({0,0,1})-origin);
 }
 out.resize(geometry.vertices.size());
 for(size_t i=0;i<geometry.vertices.size();++i){auto& v=geometry.vertices[i];Vec point{};
  for(int j=0;j<4;++j)if(v.weights[j]){const auto& a=transforms[v.bones[j]];point=point+(a.origin+a.x*v.point.x+a.y*v.point.y+a.z*v.point.z)*(v.weights[j]/255.f);}
  out[i]=point;if(!finite(point))return false;
 }
 return true;
}
}
