// automatically generated by the FlatBuffers compiler, do not modify

import com.google.flatbuffers.BaseVector;
import com.google.flatbuffers.BooleanVector;
import com.google.flatbuffers.ByteVector;
import com.google.flatbuffers.Constants;
import com.google.flatbuffers.DoubleVector;
import com.google.flatbuffers.FlatBufferBuilder;
import com.google.flatbuffers.FloatVector;
import com.google.flatbuffers.IntVector;
import com.google.flatbuffers.LongVector;
import com.google.flatbuffers.ShortVector;
import com.google.flatbuffers.StringVector;
import com.google.flatbuffers.Struct;
import com.google.flatbuffers.Table;
import com.google.flatbuffers.UnionVector;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

@SuppressWarnings("unused")
public final class Movie extends Table {
  public static void ValidateVersion() { Constants.FLATBUFFERS_23_5_26(); }
  public static Movie getRootAsMovie(ByteBuffer _bb) { return getRootAsMovie(_bb, new Movie()); }
  public static Movie getRootAsMovie(ByteBuffer _bb, Movie obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public static boolean MovieBufferHasIdentifier(ByteBuffer _bb) { return __has_identifier(_bb, "MOVI"); }
  public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
  public Movie __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public byte mainCharacterType() { int o = __offset(4); return o != 0 ? bb.get(o + bb_pos) : 0; }
  public Table mainCharacter(Table obj) { int o = __offset(6); return o != 0 ? __union(obj, o + bb_pos) : null; }
  public byte charactersType(int j) { int o = __offset(8); return o != 0 ? bb.get(__vector(o) + j * 1) : 0; }
  public int charactersTypeLength() { int o = __offset(8); return o != 0 ? __vector_len(o) : 0; }
  public ByteVector charactersTypeVector() { return charactersTypeVector(new ByteVector()); }
  public ByteVector charactersTypeVector(ByteVector obj) { int o = __offset(8); return o != 0 ? obj.__assign(__vector(o), bb) : null; }
  public ByteBuffer charactersTypeAsByteBuffer() { return __vector_as_bytebuffer(8, 1); }
  public ByteBuffer charactersTypeInByteBuffer(ByteBuffer _bb) { return __vector_in_bytebuffer(_bb, 8, 1); }
  public Table characters(Table obj, int j) { int o = __offset(10); return o != 0 ? __union(obj, __vector(o) + j * 4) : null; }
  public int charactersLength() { int o = __offset(10); return o != 0 ? __vector_len(o) : 0; }
  public UnionVector charactersVector() { return charactersVector(new UnionVector()); }
  public UnionVector charactersVector(UnionVector obj) { int o = __offset(10); return o != 0 ? obj.__assign(__vector(o), 4, bb) : null; }

  public static int createMovie(FlatBufferBuilder builder,
      byte mainCharacterType,
      int mainCharacterOffset,
      int charactersTypeOffset,
      int charactersOffset) {
    builder.startTable(4);
    Movie.addCharacters(builder, charactersOffset);
    Movie.addCharactersType(builder, charactersTypeOffset);
    Movie.addMainCharacter(builder, mainCharacterOffset);
    Movie.addMainCharacterType(builder, mainCharacterType);
    return Movie.endMovie(builder);
  }

  public static void startMovie(FlatBufferBuilder builder) { builder.startTable(4); }
  public static void addMainCharacterType(FlatBufferBuilder builder, byte mainCharacterType) { builder.addByte(0, mainCharacterType, 0); }
  public static void addMainCharacter(FlatBufferBuilder builder, int mainCharacterOffset) { builder.addOffset(1, mainCharacterOffset, 0); }
  public static void addCharactersType(FlatBufferBuilder builder, int charactersTypeOffset) { builder.addOffset(2, charactersTypeOffset, 0); }
  public static int createCharactersTypeVector(FlatBufferBuilder builder, byte[] data) { builder.startVector(1, data.length, 1); for (int i = data.length - 1; i >= 0; i--) builder.addByte(data[i]); return builder.endVector(); }
  public static void startCharactersTypeVector(FlatBufferBuilder builder, int numElems) { builder.startVector(1, numElems, 1); }
  public static void addCharacters(FlatBufferBuilder builder, int charactersOffset) { builder.addOffset(3, charactersOffset, 0); }
  public static int createCharactersVector(FlatBufferBuilder builder, int[] data) { builder.startVector(4, data.length, 4); for (int i = data.length - 1; i >= 0; i--) builder.addOffset(data[i]); return builder.endVector(); }
  public static void startCharactersVector(FlatBufferBuilder builder, int numElems) { builder.startVector(4, numElems, 4); }
  public static int endMovie(FlatBufferBuilder builder) {
    int o = builder.endTable();
    return o;
  }
  public static void finishMovieBuffer(FlatBufferBuilder builder, int offset) { builder.finish(offset, "MOVI"); }
  public static void finishSizePrefixedMovieBuffer(FlatBufferBuilder builder, int offset) { builder.finishSizePrefixed(offset, "MOVI"); }

  public static final class Vector extends BaseVector {
    public Vector __assign(int _vector, int _element_size, ByteBuffer _bb) { __reset(_vector, _element_size, _bb); return this; }

    public Movie get(int j) { return get(new Movie(), j); }
    public Movie get(Movie obj, int j) {  return obj.__assign(__indirect(__element(j), bb), bb); }
  }
  public MovieT unpack() {
    MovieT _o = new MovieT();
    unpackTo(_o);
    return _o;
  }
  public void unpackTo(MovieT _o) {
    CharacterUnion _oMainCharacter = new CharacterUnion();
    byte _oMainCharacterType = mainCharacterType();
    _oMainCharacter.setType(_oMainCharacterType);
    Table _oMainCharacterValue;
    switch (_oMainCharacterType) {
      case Character.MuLan:
        _oMainCharacterValue = mainCharacter(new Attacker());
        _oMainCharacter.setValue(_oMainCharacterValue != null ? ((Attacker) _oMainCharacterValue).unpack() : null);
        break;
      default: break;
    }
    _o.setMainCharacter(_oMainCharacter);
    CharacterUnion[] _oCharacters = new CharacterUnion[charactersLength()];
    for (int _j = 0; _j < charactersLength(); ++_j) {
      CharacterUnion _oCharactersElement = new CharacterUnion();
      byte _oCharactersElementType = charactersType(_j);
      _oCharactersElement.setType(_oCharactersElementType);
      Table _oCharactersElementValue;
      switch (_oCharactersElementType) {
        case Character.MuLan:
          _oCharactersElementValue = characters(new Attacker(), _j);
          _oCharactersElement.setValue(_oCharactersElementValue != null ? ((Attacker) _oCharactersElementValue).unpack() : null);
          break;
        default: break;
      }
      _oCharacters[_j] = _oCharactersElement;
    }
    _o.setCharacters(_oCharacters);
  }
  public static int pack(FlatBufferBuilder builder, MovieT _o) {
    if (_o == null) return 0;
    byte _mainCharacterType = _o.getMainCharacter() == null ? Character.NONE : _o.getMainCharacter().getType();
    int _mainCharacter = _o.getMainCharacter() == null ? 0 : CharacterUnion.pack(builder, _o.getMainCharacter());
    int _charactersType = 0;
    if (_o.getCharacters() != null) {
      byte[] __charactersType = new byte[_o.getCharacters().length];
      int _j = 0;
      for (CharacterUnion _e : _o.getCharacters()) { __charactersType[_j] = _o.getCharacters()[_j].getType(); _j++;}
      _charactersType = createCharactersTypeVector(builder, __charactersType);
    }
    int _characters = 0;
    if (_o.getCharacters() != null) {
      int[] __characters = new int[_o.getCharacters().length];
      int _j = 0;
      for (CharacterUnion _e : _o.getCharacters()) { __characters[_j] = CharacterUnion.pack(builder,  _o.getCharacters()[_j]); _j++;}
      _characters = createCharactersVector(builder, __characters);
    }
    return createMovie(
      builder,
      _mainCharacterType,
      _mainCharacter,
      _charactersType,
      _characters);
  }
}

