from pathlib import Path
import sys

SKILL_DIR = Path(r"C:\Users\lenovo\.agents\skills\solidworks-automation")
sys.path.insert(0, str(SKILL_DIR / "scripts"))

from sw_connect import mm
from sw_part import (
    sketch,
    sketch_rectangle,
    sketch_circle,
    extrude_boss,
    extrude_cut,
)
from sw_session import SolidWorksSession
from sw_review import run_review


OUT_DIR = Path(r"D:\fengboshi\-plan\solidworks_output\reference_bracket")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def select_plane(model, name):
    model.ClearSelection2(True)
    ok = model.Extension.SelectByID2(name, "PLANE", 0, 0, 0, False, 0, None, 0)
    if not ok:
        raise RuntimeError(f"Could not select plane: {name}")


def select_all_edges(model):
    model.ClearSelection2(True)
    ext = model.Extension
    # Box extents around the whole part; SelectByRay catches visible outer edges after rebuild.
    # This is intentionally conservative: fillets are cosmetic and may be skipped if selection fails.
    rays = [
        (0, 0, mm(20), 0, 0, -1),
        (0, 0, mm(-20), 0, 0, 1),
        (mm(20), 0, 0, -1, 0, 0),
        (mm(-20), 0, 0, 1, 0, 0),
        (0, mm(20), 0, 0, -1, 0),
        (0, mm(-20), 0, 0, 1, 0),
    ]
    for x, y, z, dx, dy, dz in rays:
        ext.SelectByRay(x, y, z, dx, dy, dz, mm(0.7), 1, True, 0, 0)


def set_material_appearance(model):
    # Dark molded plastic, close to the product photo.
    model.MaterialPropertyValues = [0.02, 0.018, 0.016, 0.0, 0.45, 0.25, 0.35, 0.0, 0.0]


def make_part():
    session = SolidWorksSession(visible=True)
    model = session.new_part()
    set_material_appearance(model)

    # Main front-view body: 21.4 x 11.8 mm, centered on origin, depth 13.5 mm.
    with sketch(model, "Front Plane") as sk:
        sketch_rectangle(model, 0, 0, mm(21.4), mm(11.8))
    main = extrude_boss(model, sk, mm(13.5), direction=True, merge=True)
    main.Name = "main_body_21p4x11p8x13p5"

    # Bottom centered lug/rail indicated as 14.6 mm in the lower view.
    with sketch(model, "Front Plane") as sk:
        sketch_rectangle(model, 0, mm(-7.1), mm(14.6), mm(2.8))
    lug = extrude_boss(model, sk, mm(13.5), direction=True, merge=True)
    lug.Name = "bottom_center_lug_14p6"

    # Central through hole and raised circular boss/ring.
    with sketch(model, "Front Plane") as sk:
        sketch_circle(model, 0, 0, mm(3.35))
        sketch_circle(model, 0, 0, mm(2.05))
    ring = extrude_boss(model, sk, mm(1.0), direction=False, merge=True)
    ring.Name = "front_circular_boss_outer_6p7_inner_4p1"

    with sketch(model, "Front Plane") as sk:
        sketch_circle(model, 0, 0, mm(2.05))
    cut = extrude_cut(model, sk, 0, direction=True)
    cut.Name = "center_through_hole_dia_4p1"

    # Two rectangular front recesses visible on the front view.
    for side, x in [("left", -6.3), ("right", 6.3)]:
        with sketch(model, "Front Plane") as sk:
            sketch_rectangle(model, mm(x), 0, mm(4.1), mm(2.4))
        recess = extrude_cut(model, sk, mm(0.9), direction=True)
        recess.Name = f"{side}_front_rect_recess"

    # Two M1.4 blind fixing holes, depth 1.5 mm, placed symmetrically.
    # The source image is blurred here, so the spacing is inferred from the side-view callout.
    for side, x in [("left", -6.4), ("right", 6.4)]:
        with sketch(model, "Front Plane") as sk:
            sketch_circle(model, mm(x), mm(-3.8), mm(0.7))
        hole = extrude_cut(model, sk, mm(1.5), direction=True)
        hole.Name = f"{side}_M1p4_blind_fixing_hole_depth_1p5"

    # Light outer edge round for molded plastic appearance.
    model.ForceRebuild3(False)
    try:
        select_all_edges(model)
        fillet = model.FeatureManager.FeatureFillet(195, mm(0.35), 0, 0, None, None, None)
        if fillet:
            fillet.Name = "small_molded_edge_fillets_R0p35"
    except Exception:
        pass

    model.ForceRebuild3(False)
    model.ViewZoomtofit2()

    sldprt = OUT_DIR / "reference_bracket_from_drawing.sldprt"
    step = OUT_DIR / "reference_bracket_from_drawing.step"
    stl = OUT_DIR / "reference_bracket_from_drawing.stl"
    session.save(model, str(sldprt))
    session.export(model, str(step))
    session.export(model, str(stl))

    report, report_path = run_review(
        model,
        OUT_DIR / "review",
        basename="reference_bracket",
        expected_outputs=[str(sldprt), str(step), str(stl)],
    )

    print("SLDPRT", sldprt)
    print("STEP", step)
    print("STL", stl)
    print("REPORT", report_path)
    print("STATUS", report["evaluation"]["status"], report["evaluation"]["score"])


if __name__ == "__main__":
    make_part()
