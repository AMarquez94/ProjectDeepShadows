#include "mcv_platform.h"
#include "render_objects.h"

CRenderCte<CCteCamera> cb_camera;
CRenderCte<CCteObject> cb_object;

struct TVtxPosClr {
	VEC3 pos;
	VEC4 color;
	TVtxPosClr() {}
	TVtxPosClr(VEC3 new_pos, VEC4 new_color) : pos(new_pos), color(new_color) {}
};

// ---------------------------------------------------

CRenderMesh* createLineZ() {
	CRenderMesh* mesh = new CRenderMesh;
	// Axis aligned X,Y,Z of sizes 1,2,3
	float vertices[] =
	{
		0.0f, 0.0f, 0.0f,  1, 1, 1, 1,
		0.0f, 0.0f, 1.0f,  1, 1, 1, 1,
	};
	if (!mesh->create(vertices, sizeof(vertices), "PosClr", CRenderMesh::LINE_LIST))
		return nullptr;
	return mesh;
}

// ---------------------------------------------------
CRenderMesh* createAxis() {
	CRenderMesh* mesh = new CRenderMesh;
	// Axis aligned X,Y,Z of sizes 1,2,3
	float vertices[] =
	{
		0.0f, 0.0f, 0.0f,  1, 0, 0, 1,
		1.0f, 0.0f, 0.0f,  1, 0, 0, 1,
		0.0f, 0.0f, 0.0f,  0, 1, 0, 1,
		0.0f, 2.0f, 0.0f,  0, 1, 0, 1,
		0.0f, 0.0f, 0.0f,  0, 0, 1, 1,
		0.0f, 0.0f, 3.0f,  0, 0, 1, 1,
	};
	if (!mesh->create(vertices, sizeof(vertices), "PosClr", CRenderMesh::LINE_LIST))
		return nullptr;
	return mesh;
}

// ---------------------------------------------------
CRenderMesh* createUnitCircleXZ(int nsamples) {
	CRenderMesh* mesh = new CRenderMesh;

	std::vector< TVtxPosClr > vtxs;
	vtxs.resize(nsamples * 2);
	auto* v = vtxs.data();
	VEC4 clr(1, 1, 1, 1);
	float du = 2.0f * (float)(M_PI) / (float)(nsamples);
	VEC3 p = getVectorFromYaw(0.0f);
	for (int i = 1; i <= nsamples; ++i) {
		*v++ = TVtxPosClr(VEC3(p.x, 0.0f, p.z), clr);
		p = getVectorFromYaw(i * du);
		*v++ = TVtxPosClr(VEC3(p.x, 0.0f, p.z), clr);
	}
	assert(v == vtxs.data() + vtxs.size());
	if (!mesh->create(vtxs.data(), vtxs.size() * sizeof(TVtxPosClr), "PosClr", CRenderMesh::LINE_LIST))
		return nullptr;
	return mesh;
}

CRenderMesh* createGridXZ(int nsteps) {
	CRenderMesh* mesh = new CRenderMesh;

	std::vector<TVtxPosClr> vtxs;
	VEC4 clr2(0.5f, 0.5f, 0.5f, 1.0f);
	VEC4 clr1(0.25f, 0.25f, 0.25f, 1.0f);
	for (int i = -nsteps; i <= nsteps; ++i) {
		VEC4 clr = (i % 5) ? clr1 : clr2;
		float fi = (float)i;
		float fnsteps = (float)nsteps;
		vtxs.emplace_back(VEC3(fi, 0, fnsteps), clr);
		vtxs.emplace_back(VEC3(fi, 0, -fnsteps), clr);
		vtxs.emplace_back(VEC3(fnsteps, 0, fi), clr);
		vtxs.emplace_back(VEC3(-fnsteps, 0, fi), clr);
	}

	if (!mesh->create(vtxs.data(), vtxs.size() * sizeof(TVtxPosClr), "PosClr", CRenderMesh::LINE_LIST))
		return nullptr;
	return mesh;
}

CRenderMesh* createCone(float fov, float dist, int steps, VEC4 clr) {
	CRenderMesh* mesh = new CRenderMesh;

	std::vector<TVtxPosClr> vtxs;

	vtxs.emplace_back(VEC3(0.f, 0.f, 0.f), clr);
	vtxs.emplace_back(VEC3(dist * sinf(-fov / 2), 0.f, dist * cosf(-fov / 2)), clr);
	vtxs.emplace_back(VEC3(0.f, 0.f, 0.f), clr);
	vtxs.emplace_back(VEC3(dist * sinf(fov / 2), 0.f, dist * cosf(fov / 2)), clr);

	float fovUnit = fov / steps;

	for (int i = -steps / 2; i < steps / 2; i++) {
		vtxs.emplace_back(VEC3(dist * sinf(fovUnit * i), 0.f, dist * cosf(fovUnit * i)), clr);
		vtxs.emplace_back(VEC3(dist * sinf(fovUnit * (i + 1)), 0.f, dist * cosf(fovUnit * (i + 1))), clr);
	}


	/*vtxs.emplace_back(VEC3(dist * cosf(-fov / 2), 0.f, dist * sinf(-fov / 2)), clr);
	vtxs.emplace_back(VEC3(dist * cosf(fov / 2), 0.f, dist * sinf(fov / 2)), clr);*/

	if (!mesh->create(vtxs.data(), vtxs.size() * sizeof(TVtxPosClr), "PosClr", CRenderMesh::LINE_LIST))
		return nullptr;
	return mesh;
}

// --------------------------
void registerMesh(CRenderMesh* new_mesh, const char* name) {
	new_mesh->setNameAndClass(name, getResourceClassOf<CRenderMesh>());
	Resources.registerResource(new_mesh);
}

bool createRenderObjects() {
	registerMesh(createAxis(), "axis.mesh");
	registerMesh(createGridXZ(20), "grid.mesh");
	registerMesh(createLineZ(), "line.mesh");
	registerMesh(createUnitCircleXZ(32), "circle_xz.mesh");
	registerMesh(createCone(deg2rad(60), 10.f, 10, VEC4(1.0f, 1.0f, 1.0f, 1.0f)), "cone_of_vision.mesh");
	registerMesh(createCone(deg2rad(30), 10.f, 10, VEC4(1.0f, 1.0f, 0.0f, 1.0f)), "cone_of_light.mesh");
	return true;
}

void destroyRenderObjects() {
}

void activateCamera(const CCamera& camera) {
	cb_camera.camera_view = camera.getView();
	cb_camera.camera_proj = camera.getProjection();
	cb_camera.camera_pos = camera.getPosition();
	cb_camera.updateGPU();
}

void setWorldTransform(MAT44 new_matrix, VEC4 new_color) {
	cb_object.obj_world = new_matrix;
	cb_object.obj_color = new_color;
	cb_object.updateGPU();
}


