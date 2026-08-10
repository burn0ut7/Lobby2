[BaseContainerProps()]
class GC_PlayerFormationDefinition
{
    [Attribute(desc: "Formation name")]
    string m_sName;

    [Attribute(uiwidget: UIWidgets.Coords, desc: "Relative unit positions")]
    ref array<vector> m_aOffsets;
}
