function parseQueryParam(path, name) {
  const qIdx = path.indexOf('?');
  if (qIdx === -1) return null;
  const parts = path.slice(qIdx + 1).split('&');
  for (let i = 0; i < parts.length; i++) {
    const eq = parts[i].indexOf('=');
    if (eq === -1) continue;
    if (parts[i].slice(0, eq) === name)
      return parts[i].slice(eq + 1);
  }
  return null;
}

export async function get() {
  const func = parseQueryParam(REQUEST.path, 'func');
  if (!func) return { error: 'missing func parameter' };

  const sep = func.indexOf(':');
  if (sep === -1) return { error: 'invalid func: expected module:function' };

  const modulePath = func.slice(0, sep);
  const fnName = func.slice(sep + 1);

  const mod = await import(modulePath);
  if (typeof mod[fnName] !== 'function')
    return { error: `${fnName} not found in ${modulePath}` };

  const argsParam = parseQueryParam(REQUEST.path, 'args');
  const args = argsParam ? JSON.parse(decodeURIComponent(argsParam)) : [];

  return mod[fnName](args[0], args[1]);
}
