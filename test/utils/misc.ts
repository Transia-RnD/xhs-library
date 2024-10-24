export function getEmittedTxnID(meta: any): string | null {
  const affectedNodes = meta.AffectedNodes
  const hookEmissions = meta.HookEmissions

  for (const node of affectedNodes) {
    if (node.CreatedNode?.LedgerEntryType === 'EmittedTxn') {
      for (const emission of hookEmissions) {
        if (
          node.CreatedNode.NewFields.EmittedTxn.EmitDetails.EmitNonce ===
          emission.HookEmission?.EmitNonce
        ) {
          return emission.HookEmission?.EmittedTxnID
        }
      }
    }
  }

  return null
}
