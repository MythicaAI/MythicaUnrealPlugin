from pxr import UsdUtils, Usd, UsdGeom, Gf; 

def convert_usd_to_usdz(source_file: str, dest_file: str) -> bool:
    return UsdUtils.CreateNewUsdzPackage(source_file, dest_file)

def create_offset_scene(source_file: str, dest_file: str, offset: tuple[float, float, float]):
    stage = Usd.Stage.CreateInMemory()
    
    root_prim = stage.DefinePrim("/Root", "Xform")
    root_prim.GetReferences().AddReference(source_file)
    
    xform = UsdGeom.Xformable(root_prim)
    xform_api = UsdGeom.XformCommonAPI(root_prim)
    xform_api.SetTranslate(Gf.Vec3d(*offset))
    
    stage.Export(dest_file)
