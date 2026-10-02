#pragma once
#include "math.hpp"
#include <vector>
#include <array>
#include <unordered_map>
namespace kf2 {
struct BoneAtom {float x{},y{},z{},w{1};Vec translation{};float scale{1};};
inline Vec transformBone(const BoneAtom& b,Vec p){
 const Vec q{b.x,b.y,b.z};const auto t=cross(q,p)*2.f;
 return (p+t*b.w+cross(q,t))*b.scale+b.translation;
}
inline Vec transformPoint(const Matrix& m,Vec p){return {
 p.x*m.m[0]+p.y*m.m[4]+p.z*m.m[8]+m.m[12],
 p.x*m.m[1]+p.y*m.m[5]+p.z*m.m[9]+m.m[13],
 p.x*m.m[2]+p.y*m.m[6]+p.z*m.m[10]+m.m[14]};}
struct ContourSegment {float x1{},y1{},x2{},y2{};};
inline int silhouetteLod(int predicted,int count){return count>0&&count<=8?std::clamp(predicted,0,count-1):-1;}
// Rasterize the union of projected triangles: no wireframe diagonals or hidden
// limb edges leak through the body. Preserve holes between arms/legs.
class SilhouetteMask {
 int width{},height{};float left{},top{},step{1};std::vector<unsigned char> pixels;
 bool filled(int x,int y)const{return x>=0&&y>=0&&x<width&&y<height&&pixels[y*width+x]!=0;}
public:
 bool begin(float x0,float y0,float x1,float y1){
  if(!std::isfinite(x0+y0+x1+y1)||x1<=x0||y1<=y0)return false;
  left=std::floor(x0)-1;top=std::floor(y0)-1;
  step=std::max(1.f,std::max(x1-left+2,y1-top+2)/384.f);
  width=std::clamp(int(std::ceil((x1-left+2)/step)),1,384);height=std::clamp(int(std::ceil((y1-top+2)/step)),1,384);
  pixels.assign(size_t(width)*height,0);return true;
 }
 void triangle(Vec a,Vec b,Vec c){
  if(!finite(a)||!finite(b)||!finite(c)||std::abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))<.001f)return;
  std::array<Vec,3> v{a,b,c};for(auto& p:v){p.x=(p.x-left)/step;p.y=(p.y-top)/step;}
  int y0=int(std::clamp(std::ceil(std::min({v[0].y,v[1].y,v[2].y})-.5f),0.f,float(height)));
  int y1=int(std::clamp(std::floor(std::max({v[0].y,v[1].y,v[2].y})-.5f),-1.f,float(height-1)));
  for(int y=y0;y<=y1;++y){float lo=1e30f,hi=-1e30f,scan=y+.5f;
   for(int i=0;i<3;++i){auto p=v[i],q=v[(i+1)%3];if((p.y<=scan&&q.y>scan)||(q.y<=scan&&p.y>scan)){float x=p.x+(q.x-p.x)*(scan-p.y)/(q.y-p.y);lo=std::min(lo,x);hi=std::max(hi,x);}}
   if(lo>hi)continue;
   const int start=int(std::clamp(std::ceil(lo-.5f),0.f,float(width))),end=int(std::clamp(std::floor(hi-.5f),-1.f,float(width-1)));
   if(start<=end)std::fill(pixels.begin()+y*width+start,pixels.begin()+y*width+end+1,1);
  }
 }
 std::vector<ContourSegment> edges()const{
  std::vector<ContourSegment> out;
  for(int y=0;y<=height;++y)for(int x=0;x<width;){
   const int side=int(filled(x,y))-int(filled(x,y-1));if(!side){++x;continue;}int start=x++;
   while(x<width&&int(filled(x,y))-int(filled(x,y-1))==side)++x;
   out.push_back({left+start*step,top+y*step,left+x*step,top+y*step});
  }
  for(int x=0;x<=width;++x)for(int y=0;y<height;){
   const int side=int(filled(x,y))-int(filled(x-1,y));if(!side){++y;continue;}int start=y++;
   while(y<height&&int(filled(x,y))-int(filled(x-1,y))==side)++y;
   out.push_back({left+x*step,top+start*step,left+x*step,top+y*step});
  }
  return out;
 }
};
struct OwnedContourSegment {ContourSegment line;unsigned owner{};};
class DepthSilhouetteMask {
 struct Pixel {float inverseDepth{};unsigned owner{};};
 int width{},height{},minX{},minY{},maxX{},maxY{};float step{1};std::vector<Pixel> pixels;
 Pixel pixel(int x,int y)const{return x>=0&&y>=0&&x<width&&y<height?pixels[y*width+x]:Pixel{};}
 static unsigned boundary(Pixel a,Pixel b){
  if(a.owner==b.owner)return 0;
  if(!a.owner)return b.owner;if(!b.owner)return a.owner;
  return a.inverseDepth>=b.inverseDepth?a.owner:b.owner;
 }
public:
 bool begin(float w,float h){
  if(!std::isfinite(w+h)||w<1||h<1)return false;
  step=std::max(1.f,std::max(w/1920.f,h/1080.f));width=int(std::ceil(w/step));height=int(std::ceil(h/step));
  minX=width;minY=height;maxX=maxY=-1;
  pixels.assign(size_t(width)*height,{});return true;
 }
 unsigned ownerAt(float x,float y)const{return pixel(int(x/step),int(y/step)).owner;}
 void triangle(Vec a,Vec b,Vec c,unsigned owner){
  if(!owner||!finite(a)||!finite(b)||!finite(c)||a.z==0||b.z==0||c.z==0||std::signbit(a.z)!=std::signbit(b.z)||std::signbit(a.z)!=std::signbit(c.z))return;
  std::array<Vec,3> v{a,b,c};for(auto& p:v){p.x/=step;p.y/=step;p.z=1/std::abs(p.z);}
  const float area=(v[1].x-v[0].x)*(v[2].y-v[0].y)-(v[1].y-v[0].y)*(v[2].x-v[0].x);if(!std::isfinite(area)||std::abs(area)<.0001f)return;
  const float dzdx=((v[1].z-v[0].z)*(v[2].y-v[0].y)-(v[2].z-v[0].z)*(v[1].y-v[0].y))/area;
  const float dzdy=((v[1].x-v[0].x)*(v[2].z-v[0].z)-(v[2].x-v[0].x)*(v[1].z-v[0].z))/area;
  int y0=int(std::clamp(std::ceil(std::min({v[0].y,v[1].y,v[2].y})-.5f),0.f,float(height)));
  int y1=int(std::clamp(std::floor(std::max({v[0].y,v[1].y,v[2].y})-.5f),-1.f,float(height-1)));
  for(int y=y0;y<=y1;++y){float lo=1e30f,hi=-1e30f,scan=y+.5f;
   for(int i=0;i<3;++i){auto p=v[i],q=v[(i+1)%3];if((p.y<=scan&&q.y>scan)||(q.y<=scan&&p.y>scan)){const float x=p.x+(q.x-p.x)*(scan-p.y)/(q.y-p.y);lo=std::min(lo,x);hi=std::max(hi,x);}}
   if(lo>hi)continue;int start=int(std::clamp(std::ceil(lo-.5f),0.f,float(width))),end=int(std::clamp(std::floor(hi-.5f),-1.f,float(width-1)));
   if(start<=end){minX=std::min(minX,start);maxX=std::max(maxX,end);minY=std::min(minY,y);maxY=std::max(maxY,y);}
   float z=v[0].z+dzdx*(start+.5f-v[0].x)+dzdy*(scan-v[0].y);
   for(int x=start;x<=end;++x,z+=dzdx){auto& dst=pixels[y*width+x];if(z>dst.inverseDepth){dst={z,owner};}}
  }
 }
 std::vector<OwnedContourSegment> edges()const{
  struct Edge {int a{},b{};unsigned owner{};bool used{};};std::vector<Edge> edges;
  if(maxX<minX||maxY<minY)return {};
  auto vertex=[&](int x,int y){return y*(width+1)+x;};
  for(int y=minY;y<=maxY+1;++y)for(int x=minX;x<=maxX;){auto id=boundary(pixel(x,y),pixel(x,y-1));if(!id){++x;continue;}int start=x++;
   while(x<=maxX&&boundary(pixel(x,y),pixel(x,y-1))==id)++x;edges.push_back({vertex(start,y),vertex(x,y),id});}
  for(int x=minX;x<=maxX+1;++x)for(int y=minY;y<=maxY;){auto id=boundary(pixel(x,y),pixel(x-1,y));if(!id){++y;continue;}int start=y++;
   while(y<=maxY&&boundary(pixel(x,y),pixel(x-1,y))==id)++y;edges.push_back({vertex(x,start),vertex(x,y),id});}
  std::unordered_multimap<std::uint64_t,int> adjacency;adjacency.reserve(edges.size()*2);
  auto key=[](unsigned owner,int v){return (std::uint64_t(owner)<<32)|unsigned(v);};
  for(int i=0;i<int(edges.size());++i){adjacency.emplace(key(edges[i].owner,edges[i].a),i);adjacency.emplace(key(edges[i].owner,edges[i].b),i);}
  std::vector<OwnedContourSegment> out;
  auto position=[&](int v){return Vec{float(v%(width+1))*step,float(v/(width+1))*step,0};};
  auto emit=[&](const std::vector<int>& path,unsigned owner){
   if(path.size()<2)return;std::vector<std::pair<size_t,size_t>> pending;
   if(path.front()==path.back()&&path.size()>3){size_t middle=path.size()/2;pending={{0,middle},{middle,path.size()-1}};}else pending={{0,path.size()-1}};
   // Keep the previous screen-space error budget while sampling the mesh
   // at twice the resolution. Smooth diagonal contours without extra draw calls.
   const float toleranceSquared=step*step*2.56f;
   while(!pending.empty()){auto [start,end]=pending.back();pending.pop_back();auto a=position(path[start]),b=position(path[end]),d=b-a;float squared=dot(d,d),worst=0;size_t split=start;
    for(size_t i=start+1;i<end;++i){auto p=position(path[i]),delta=p-a;float t=squared>0?std::clamp(dot(delta,d)/squared,0.f,1.f):0;auto error=delta-d*t;float distance=dot(error,error);if(distance>worst){worst=distance;split=i;}}
    if(worst>toleranceSquared&&split>start){pending.emplace_back(start,split);pending.emplace_back(split,end);}else out.push_back({{a.x,a.y,b.x,b.y},owner});
   }
  };
  auto trace=[&](int edgeIndex,int start){auto owner=edges[edgeIndex].owner;std::vector<int> path{start};int current=start,index=edgeIndex;
   for(size_t n=0;n<=edges.size();++n){auto& e=edges[index];if(e.used)break;e.used=true;current=e.a==current?e.b:e.a;path.push_back(current);if(current==start)break;
    auto k=key(owner,current);if(adjacency.count(k)!=2)break;auto range=adjacency.equal_range(k);int next=-1;for(auto it=range.first;it!=range.second;++it)if(!edges[it->second].used){next=it->second;break;}if(next<0)break;index=next;}
   emit(path,owner);
  };
  for(int i=0;i<int(edges.size());++i)if(!edges[i].used){auto& e=edges[i];if(adjacency.count(key(e.owner,e.a))!=2)trace(i,e.a);else if(adjacency.count(key(e.owner,e.b))!=2)trace(i,e.b);}
  for(int i=0;i<int(edges.size());++i)if(!edges[i].used)trace(i,edges[i].a);
  return out;
 }
};

}
