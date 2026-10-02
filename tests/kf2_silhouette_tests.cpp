#include "silhouette.hpp"
#include <cstdio>
int main(){using namespace kf2;int failures=0;auto check=[&](bool v,const char* name){if(!v){std::fprintf(stderr,"FAIL: %s\n",name);++failures;}};
 BoneAtom rotated{0,0,std::sqrt(.5f),std::sqrt(.5f),{2,3,4},2};auto p=transformBone(rotated,{1,0,0});check(length(p-Vec{2,5,4})<.001f,"bone rotation scale and translation");
 BoneAtom bind{0,0,0,1,{-5,0,0},1},pose{0,0,0,1,{10,0,0},1};check(length(transformBone(pose,transformBone(bind,{7,1,2}))-Vec{12,1,2})<.001f,"inverse bind then animated pose");
 SilhouetteMask mask;check(mask.begin(0,0,10,10),"initialize mask");mask.triangle({0,0,1},{10,0,1},{0,10,1});mask.triangle({10,0,1},{10,10,1},{0,10,1});auto edges=mask.edges();check(edges.size()==4,"two triangles form only four exterior edges");
 float perimeter=0;for(auto e:edges)perimeter+=std::hypot(e.x2-e.x1,e.y2-e.y1);check(std::abs(perimeter-40)<.01f,"rectangle perimeter has no diagonal");
 // Two separated legs remain two silhouettes; overlapping triangles never add internal edges.
 mask.begin(0,0,12,10);for(float x:{0.f,8.f}){mask.triangle({x,0,1},{x+4,0,1},{x,10,1});mask.triangle({x+4,0,1},{x+4,10,1},{x,10,1});}
 check(mask.edges().size()==8,"preserve gap between limbs");mask.triangle({0,0,1},{4,0,1},{0,10,1});check(mask.edges().size()==8,"overlapping body surfaces have no duplicate contours");
 mask.begin(0,0,10,10);mask.triangle({1,1,1},{2,2,1},{3,3,1});check(mask.edges().empty(),"ignore collapsed or severed triangles");
 mask.begin(0,0,10000,10000);mask.triangle({-1000000,0,1},{1000000,0,1},{0,1000000,1});check(mask.edges().size()<1600,"bound close-up raster complexity and clip enormous triangles");
 check(!mask.begin(0,0,0,10),"reject empty projected bounds");
 std::vector<int> data;check(!readGeometryArray<int>(0,4,10,data),"reject missing CPU buffers");check(!readGeometryArray<int>(1,11,10,data),"reject oversized geometry before memory access");

 check(silhouetteLod(3,4)==3&&silhouetteLod(0,4)==0,"outline follows renderer LOD across distance changes");
 check(silhouetteLod(8,4)==3&&silhouetteLod(-1,4)==0&&silhouetteLod(1,0)==-1,"LOD requests are bounded and missing meshes rejected");
 DepthSilhouetteMask depth;depth.begin(32,32);
 auto rect=[&](float x0,float y0,float x1,float y1,float z,unsigned id){depth.triangle({x0,y0,z},{x1,y0,z},{x0,y1,z},id);depth.triangle({x1,y0,z},{x1,y1,z},{x0,y1,z},id);};
 rect(2,2,22,22,20,1);rect(8,0,14,26,10,2);
 check(depth.ownerAt(10,10)==2&&depth.ownerAt(4,10)==1,"front Zed covers rear Zed while uncovered portions remain");
 bool hiddenRear=false;for(auto e:depth.edges())if(e.owner==1){float x=(e.line.x1+e.line.x2)/2,y=(e.line.y1+e.line.y2)/2;if(x>8&&x<14&&y>0&&y<26)hiddenRear=true;}
 check(!hiddenRear,"no rear outline is drawn inside front Zed");
 depth.begin(32,32);rect(2,2,22,22,10,2);rect(2,2,22,22,20,1);auto hidden=depth.edges();
 check(!hidden.empty()&&std::all_of(hidden.begin(),hidden.end(),[](auto e){return e.owner==2;}),"fully hidden rear outline disappears independent of draw order");
 depth.begin(32,32);rect(2,2,22,22,-10,2);rect(2,2,22,22,-20,1);check(depth.ownerAt(10,10)==2,"scope depth uses positive camera distance for occlusion");
 depth.begin(32,32);rect(0,0,32,32,10,1);depth.triangle({0,0,2},{32,0,40},{0,32,2},2);
 check(depth.ownerAt(2,2)==2&&depth.ownerAt(28,1)==1,"perspective depth varies across a tilted character surface");
 // One-pixel details survive at 1080p, where the old half-resolution mask lost them.
 depth.begin(1920,1080);rect(3,3,4,20,10,1);check(depth.ownerAt(3.5f,10)==1&&depth.ownerAt(4.5f,10)==0,"full-resolution mask retains thin head and foot features");
 // Curved crowds used to issue one native call for every raster stair step.
 depth.begin(1920,1080);size_t oldSegments=0;
 for(unsigned id=1;id<=20;++id){float cx=150+float((id-1)%5)*340,cy=130+float((id-1)/5)*250;mask.begin(cx-60,cy-110,cx+60,cy+110);
  for(int i=0;i<64;++i){float a=i*2*std::numbers::pi_v<float>/64,b=(i+1)*2*std::numbers::pi_v<float>/64;Vec center{cx,cy,100+float(id)},p{cx+60*std::cos(a),cy+110*std::sin(a),center.z},q{cx+60*std::cos(b),cy+110*std::sin(b),center.z};mask.triangle(center,p,q);depth.triangle(center,p,q,id);}
  oldSegments+=mask.edges().size();
 }
 auto reduced=depth.edges().size();std::printf("20 curved silhouettes: old %zu segments, simplified %zu\n",oldSegments,reduced);
 check(reduced*4<oldSegments,"simplified contours cut native draw calls by over 75 percent on curved crowd fixture");
 return failures?1:0;}
