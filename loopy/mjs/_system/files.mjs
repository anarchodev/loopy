export function saveFile(path, content) {
    KV.setItem(`_js.f.user_files/${path}`, content);
    KV.removeItem(`_js.b.user_files/${path}`, content);
    return [path, content];
}

export function getFile(path) {
    return KV.getItem(`_js.f.user_files/${path}`)
}
