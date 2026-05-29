.pragma library

function stripHtml(html) {
    var text = html
    text = text.replace(/<br\s*\/?>/gi, "\n")
    text = text.replace(/<\/p>/gi, "\n\n")
    text = text.replace(/<[^>]*>/g, "")
    // Named entities
    text = text.replace(/&amp;/g, "&")
    text = text.replace(/&lt;/g, "<")
    text = text.replace(/&gt;/g, ">")
    text = text.replace(/&quot;/g, "\"")
    text = text.replace(/&#39;|&apos;/g, "'")
    text = text.replace(/&nbsp;/g, " ")
    text = text.replace(/&copy;/g, "\u00A9")
    text = text.replace(/&mdash;/g, "\u2014")
    text = text.replace(/&ndash;/g, "\u2013")
    text = text.replace(/&hellip;/g, "\u2026")
    // Numeric entities: &#xHEX; and &#DEC;
    text = text.replace(/&#x([0-9a-fA-F]+);/g, function(match, hex) {
        return String.fromCharCode(parseInt(hex, 16))
    })
    text = text.replace(/&#(\d+);/g, function(match, dec) {
        return String.fromCharCode(parseInt(dec, 10))
    })
    return text
}
