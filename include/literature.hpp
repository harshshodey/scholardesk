#pragma once
#include <array>
#include <string>

namespace scholar {
// Verbatim short passages from the linked human-authored works.
// Surrounding quotation marks and ASCII apostrophes are presentation typography.
// Fragments are explicitly marked; see LITERARY_SOURCES.md for verification and context.
struct LiteraryQuote {
    const char* id;
    const char* text;
    const char* author;
    const char* work;
    const char* locator;
    const char* url;
    const char* context;
    bool fragment;
};
inline const std::array<LiteraryQuote, 12>& literaryQuotes() {
    static const std::array<LiteraryQuote, 12> quotes{{
        {"possibility", "I dwell in Possibility", "Emily Dickinson", "I dwell in Possibility - (466)", "Opening-line fragment; composed c. 1862",
         "https://www.poetryfoundation.org/poems/52197/i-dwell-in-possibility-466",
         "A short fragment of the poem's opening line. The full poem develops a house as an image of poetic possibility.", true},
        {"beginning", "Begin at the beginning", "Lewis Carroll", "Alice's Adventures in Wonderland", "Chapter XII (1865); speech fragment",
         "https://standardebooks.org/ebooks/lewis-carroll/alices-adventures-in-wonderland/john-tenniel/text/chapter-12",
         "The King tells the White Rabbit where to begin reading. This is a fragment of his longer instruction.", true},
        {"curiosity", "Curiouser and curiouser!", "Lewis Carroll", "Alice's Adventures in Wonderland", "Chapter II (1865)",
         "https://standardebooks.org/ebooks/lewis-carroll/alices-adventures-in-wonderland/john-tenniel/text/chapter-2",
         "Alice exclaims after another surprising change in size. Carroll's deliberately unusual grammar is preserved.", false},
        {"identity", "Who in the world am I?", "Lewis Carroll", "Alice's Adventures in Wonderland", "Chapter II (1865); speech excerpt",
         "https://standardebooks.org/ebooks/lewis-carroll/alices-adventures-in-wonderland/john-tenniel/text/chapter-2",
         "Alice wonders whether she has become someone else. Here it accompanies the directory and search.", true},
        {"cheshire", "we're all mad here.", "Lewis Carroll", "Alice's Adventures in Wonderland", "Chapter VI (1865); speech excerpt",
         "https://standardebooks.org/ebooks/lewis-carroll/alices-adventures-in-wonderland/john-tenniel/text/chapter-6",
         "A line spoken by the Cheshire Cat in a nonsense fantasy, included as literary humour rather than a description of any student.", true},
        {"work", "I like work: it fascinates me. I can sit and look at it for hours.", "Jerome K. Jerome", "Three Men in a Boat", "Chapter XV (1889)",
         "https://standardebooks.org/ebooks/jerome-k-jerome/three-men-in-a-boat/text/chapter-15",
         "The narrator jokes about work while discussing the division of labour on the boat. The colon matches this edition.", false},
        {"diary", "I never travel without my diary. One should always have something sensational to read in the train.", "Oscar Wilde", "The Importance of Being Earnest", "Act II (1895); Gwendolen's dialogue",
         "https://en.wikisource.org/wiki/The_Importance_of_Being_Earnest/Act_II",
         "Gwendolen produces her diary during her conversation with Cecily. The joke belongs to a fictional character in Wilde's play.", false},
        {"reading", "I declare after all there is no enjoyment like reading!", "Jane Austen", "Pride and Prejudice", "Chapter XI (1813); Caroline Bingley's dialogue",
         "https://standardebooks.org/ebooks/jane-austen/pride-and-prejudice/text/chapter-11",
         "Caroline Bingley says this while trying to attract Darcy's attention, then soon sets her book aside. Its original context is ironic.", false},
        {"strive", "To strive, to seek, to find, and not to yield.", "Alfred, Lord Tennyson", "Ulysses", "Final line (1842)",
         "https://www.poetryfoundation.org/poems/45392/ulysses",
         "The closing line is spoken by Ulysses in a dramatic monologue. It is a character's declaration, not a grading policy.", false},
        {"done", "Well done is better than well said.", "Benjamin Franklin", "Poor Richard's Almanack", "1737; reproduced by The Huntington",
         "https://www.huntington.org/sites/default/files/pdfs/lhthfranklin.pdf",
         "A proverb printed in Franklin's almanac, reproduced on page 8 of The Huntington's teaching resource. Credit records publication, not a claim that Franklin coined it.", false},
        {"patience", "Learn to labor and to wait.", "Henry Wadsworth Longfellow", "A Psalm of Life", "Final line (1838)",
         "https://www.poetryfoundation.org/poems/44644/a-psalm-of-life",
         "The final line of the poem. This is encouragement for learning, without making light of attendance or academic difficulties.", false},
        {"smart", "I am always smart!", "Oscar Wilde", "The Importance of Being Earnest", "Act I (1895); Gwendolen's dialogue",
         "https://en.wikisource.org/wiki/The_Importance_of_Being_Earnest/Act_I",
         "Gwendolen replies to Algernon's compliment. In the scene, smart can mean fashionable; the double meaning supplies the joke.", false}
    }};
    return quotes;
}
inline std::string literaryCredit(std::size_t index) {
    const auto& q = literaryQuotes().at(index);
    return std::string(q.author) + " | " + q.work + (q.fragment ? " (excerpt)" : "");
}
inline std::string literaryDisplay(std::size_t index) {
    return std::string("\"") + literaryQuotes().at(index).text + "\"";
}
}
